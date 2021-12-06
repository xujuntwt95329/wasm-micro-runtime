/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_export.h"
#include "bh_read_file.h"
#include "pthread.h"

#define APP_NUM 5

/* In this thread, we create totally new module_instance and new exec_env
    from the same wasm module, so this is a new wasm app.
   The exec_env is created and also executed in the same thread */
void *
thread_to_execute_new_app(void *arg)
{
    uint32 stack_size = 16 * 1024, heap_size = 65536 * 1024;
    char *wasm_file = (char *)arg;
    wasm_module_t new_app_module;
    wasm_module_inst_t new_app_inst;
    wasm_exec_env_t new_app_exec_env;
    wasm_function_inst_t func;
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    char error_buf[128] = { 0 };
    uint32 argv[2];

    if (!wasm_runtime_init_thread_env()) {
        printf("failed to initialize thread environment");
        return NULL;
    }

    /* Why we need to create a new wasm module? Usually we can use the same
        wasm module for several apps, however, when debugging the wasm app,
        we may set breakpoints which may change the bytecode, and the bytecode
        is actually stored in the wasm_module. So we need to create a new one
        when using debugging feature, otherwise the breakpoints set in one app
        will influence the other apps who share the same wasm module */
    if (!(wasm_file_buf =
              (uint8 *)bh_read_file_to_buffer(wasm_file, &wasm_file_size)))
        goto fail1;

    if (!(new_app_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                             error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail2;
    }

    if (!(new_app_inst =
              wasm_runtime_instantiate(new_app_module, stack_size, heap_size,
                                       error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail3;
    }

    if (!(new_app_exec_env =
              wasm_runtime_create_exec_env(new_app_inst, stack_size))) {
        printf("failed to create exec_env\n");
        wasm_runtime_deinstantiate(new_app_inst);
        goto fail4;
    }

    /* Start debug instance for this exec_env */
    wasm_runtime_start_debug_instance(new_app_exec_env);

    func =
        wasm_runtime_lookup_function(new_app_inst, "__main_argc_argv", NULL);
    if (!func) {
        printf("failed to lookup function sum");
        goto fail5;
    }

    argv[0] = 0;
    argv[1] = 0;

    if (!wasm_runtime_call_wasm(new_app_exec_env, func, 2, argv)) {
        printf("%s\n", wasm_runtime_get_exception(new_app_inst));
    }

fail5:
    wasm_runtime_destroy_exec_env(new_app_exec_env);

fail4:
    wasm_runtime_deinstantiate(new_app_inst);

fail3:
    wasm_runtime_unload(new_app_module);

fail2:
    wasm_runtime_free(wasm_file_buf);

fail1:
    wasm_runtime_destroy_thread_env();
    return NULL;
}


typedef struct ThreadArgs {
    wasm_exec_env_t exec_env;
    int start;
    int length;
} ThreadArgs;

/* In this thread, we use the exec_env created in main thread,
    and execute the main function in the wasm app */
void *
thread_to_execute_main_func(void *arg)
{
    ThreadArgs *thread_arg = (ThreadArgs *)arg;
    wasm_exec_env_t exec_env = thread_arg->exec_env;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_function_inst_t func;
    uint32 argv[2];

    if (!wasm_runtime_init_thread_env()) {
        printf("failed to initialize thread environment");
        return NULL;
    }

    func =
        wasm_runtime_lookup_function(module_inst, "__main_argc_argv", NULL);
    if (!func) {
        printf("failed to lookup function sum");
        wasm_runtime_destroy_thread_env();
        return NULL;
    }

    /* Main function need two parameter: argc and argv
        In our wasm app these two parameters are not used,
        so we pass two zero here to pass the signature validation */
    argv[0] = 0;
    argv[1] = 0;

    /* call the WASM function */
    if (!wasm_runtime_call_wasm(exec_env, func, 2, argv)) {
        printf("%s\n", wasm_runtime_get_exception(module_inst));
        wasm_runtime_destroy_thread_env();
        return NULL;
    }

    wasm_runtime_destroy_thread_env();
    return (void *)(uintptr_t)argv[0];
}

/* In this thread, we use the exec_env created in main thread,
    and execute the sum function in the wasm app */
void *
thread_to_execute_sum_func(void *arg)
{
    ThreadArgs *thread_arg = (ThreadArgs *)arg;
    wasm_exec_env_t exec_env = thread_arg->exec_env;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_function_inst_t func;
    uint32 argv[2];

    if (!wasm_runtime_init_thread_env()) {
        printf("failed to initialize thread environment");
        return NULL;
    }

    func = wasm_runtime_lookup_function(module_inst, "sum", NULL);
    if (!func) {
        printf("failed to lookup function sum");
        wasm_runtime_destroy_thread_env();
        return NULL;
    }
    argv[0] = thread_arg->start;
    argv[1] = thread_arg->length;

    /* call the WASM function */
    if (!wasm_runtime_call_wasm(exec_env, func, 2, argv)) {
        printf("%s\n", wasm_runtime_get_exception(module_inst));
        wasm_runtime_destroy_thread_env();
        return NULL;
    }

    wasm_runtime_destroy_thread_env();
    return (void *)(uintptr_t)argv[0];
}

int
main(int argc, char *argv[])
{
    char *wasm_file = "wasm-apps/test.wasm";
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size, i = 0;
    uint32 stack_size = 16 * 1024, heap_size = 65536 * 1024;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    RuntimeInitArgs init_args;
    ThreadArgs thread_arg;
    pthread_t tid, tid_new_app[APP_NUM];
    uint32 *result = NULL;
    char error_buf[128] = { 0 };

    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = malloc;
    init_args.mem_alloc_option.allocator.realloc_func = realloc;
    init_args.mem_alloc_option.allocator.free_func = free;

    /* Init runtime with debugging support, the initial debug port is 1234 */
    init_args.instance_port = 1234;
    strcpy(init_args.ip_addr, "0.0.0.0");

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return -1;
    }

    {
        /* Launch several thread to execute several new apps,
            then you can debug all of them in different lldb instances
           Their port will be 1234 ~ (1234 + APP_NUM -1) */
        for (i = 0; i < APP_NUM; i++) {
            if (0 != pthread_create(&tid_new_app[i], NULL,
                                    thread_to_execute_new_app,
                                    (void *)wasm_file)) {
                printf("New app [%d] not launched.\n", i);
            }
        }
    }

    /************ Then create a exec_env in main thread *************/

    /* Sleep for a while so that previous apps have got the port
        1234 ~ (1234 + APP_NUM -1), so the next app will get a port
        of (1234 + APP_NUM) (default is 1239)
       If we don't sleep here, then the port of the next app may be
        any one from 1234 ~ 1239 */
    sleep(1);

    /* load WASM byte buffer from WASM bin file */
    if (!(wasm_file_buf =
              (uint8 *)bh_read_file_to_buffer(wasm_file, &wasm_file_size)))
        goto fail1;

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail2;
    }

    /* instantiate the module */
    if (!(wasm_module_inst =
              wasm_runtime_instantiate(wasm_module, stack_size, heap_size,
                                       error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail3;
    }

    /* Create the exec_env, it will get a debug port:
        1234 + APP_NUM (default is 1239) */
    if (!(exec_env =
              wasm_runtime_create_exec_env(wasm_module_inst, stack_size))) {
        printf("failed to create exec_env\n");
        goto fail4;
    }

    /* Start debug instance for this exec_env */
    wasm_runtime_start_debug_instance(exec_env);

    thread_arg.start = 0;
    thread_arg.length = 10;

    thread_arg.exec_env = exec_env;

    /* Create a thread to execute the "sum" function, use the exec_env
        created in this thread */
    if (0 != pthread_create(&tid, NULL,
                            thread_to_execute_sum_func,
                            &thread_arg)) {
        printf("failed to create thread.\n");
        goto fail5;
    }

    pthread_join(tid, (void **)&result);
    printf("[Success] Got sum result from the other thread: %d\n",
           (uint32)(uintptr_t)result);

    /* Create a thread to execute the "main" function, use the exec_env
        created in this thread */
    if (0 != pthread_create(&tid, NULL,
                            thread_to_execute_main_func, &thread_arg)) {
        printf("failed to create thread.\n");
        goto fail5;
    }

    pthread_join(tid, NULL);

    for (i = 0; i < APP_NUM; i++) {
        pthread_join(tid_new_app[i], NULL);
    }

fail5:
    wasm_runtime_destroy_exec_env(exec_env);

fail4:
    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail3:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

fail2:
    /* free the file buffer */
    wasm_runtime_free(wasm_file_buf);

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();
    return 0;
}

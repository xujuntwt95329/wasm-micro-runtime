/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

int
sum(int start, int length)
{
    int sum = 0, i;

    for (i = start; i < start + length; i++) {
        sum += i;
    }

    return sum;
}


#include "stdio.h"
#include "stdlib.h"

typedef struct test_inner_struct {
    int yy[10];
} test_inner_struct;

typedef struct test_struct {
    int x;
    test_inner_struct y[10];
} test_struct;

/* test globals */
int global_simple_var = 5;
test_struct global_complex_var;

void func_no_return(int x)
{
    printf("call func_no_return with x=%d\n", x);
}

void func_return(int x)
{
    printf("call func_no_return with x=%d\n", x);
    //int y = x * 2 + 1;
}

void func(int *x)
{
    *x = 100;
}

typedef void (*test_func_pointer_type)(int);

int main(int argc, char **argv) {
    /* Test local variables */
    test_struct local_struct;
    int local_int;
    double local_double;
    int *local_ptr;
    test_func_pointer_type f;

    /* test struct field operations */
    local_struct.x = global_simple_var++;
    local_struct.y[1].yy[2] = 123;

    /* test function call */
    func_no_return(10);
    func_no_return(global_simple_var);

    func_return(10);

    /* test pointer operation */
    local_ptr = malloc(sizeof(int));

    *local_ptr = 42;
    printf("*local_ptr = %d", *local_ptr);

    /* test function pointers */
    f = func;

    f(local_ptr);
    printf("*local_ptr = %d", *local_ptr);

    /* test for loop */
    for (int i = 0; i < 10; i++) {
        /* test if else */
        if (i < 5) {
            local_int = local_struct.y[i].yy[i];
        }
        else {
            /* test switch */
            switch (i)
            {
            case 5:
            case 6:
                local_double = (double)local_struct.y[i].yy[i];
                break;
            case 7:
            case 8:
                *local_ptr = (double)local_struct.y[i].yy[i];
                /* no break */
            default:
                /* 9 */
                *local_ptr = (double)local_struct.y[i].yy[i];
                break;
            }
        }
    }

    free(local_ptr);
}

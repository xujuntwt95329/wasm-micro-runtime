# Debug a wasm-app in an embedded WAMR

This sample demonstrates how to debug wasm-app in embedded WAMR, the scenario:

- (1 + APP_NUM(default is 5)) wasm apps in the embedded runtime
- the first one is created in main thread, and executed in other threads
- others are created and executed in the same thread

We can debug all these wasm apps using several lldb instance.

## Build

``` bash
mkdir build && cd build
cmake ..
make
# This will build both the runtime and wasm application
```

## Run

1. launch the runtime
``` bash
./debuggable_runtime
```

2. launch a lldb instance to connect to any one app
``` bash
lldb
(lldb) process connect -p wasm connect://localhost:1234
```

The first APP_NUM(5) apps are all the same, the last one is special, the exec_env is created in a thread, but the executions are in other threads, the port of this special app should be `1239`:

``` bash
lldb
(lldb) process connect -p wasm connect://localhost:1239
```

You can see the current function is `sum`, you can create a breakpoint on `main` function and continue execution
``` bash
b main
c
```

Then you will hit the breakpoint in function `main`, and note that `main` is executed in different native thread from function `sum`, but when you use `thread list` to check the thread, you will see the same thread, and the thread id is actually the address of its exec_env

> Note: if you finished debugging one app and it is exited normally, don't connect this lldb to other app again, **must** exit current lldb and start a new one to debug another app.

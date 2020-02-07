
WAMR_DIR=${PWD}/../../..

/opt/wasi-sdk/bin/clang     \
        --target=wasm32 -O0 \
        -z stack-size=4096 -Wl,--initial-memory=65536 \
        --sysroot=${WAMR_DIR}/wamr-sdk/app/libc-builtin-sysroot    \
        -Wl,--allow-undefined-file=${WAMR_DIR}/wamr-sdk/app/libc-builtin-sysroot/share/defined-symbols.txt \
        -Wl,--export=sum, \
        -Wl,--no-threads,--strip-all,--no-entry \
        -nostdlib -o sum.wasm sum.c
#include "ft2_main_bridge.h"

#define main ft2_native_main
#include "../../src/ft2_main.c"
#undef main

void wasm_initialize_vars(void)
{
    initializeVars();
}

void wasm_clean_up(void)
{
    cleanUpAndExit();
}

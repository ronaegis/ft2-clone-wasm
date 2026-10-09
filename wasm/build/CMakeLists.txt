cmake_minimum_required(VERSION 3.10)

project(ft2-wasm)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../web")

# Gather original FT2 sources and exclude the native entry point
file(GLOB ft2_CORE "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c")
list(FILTER ft2_CORE EXCLUDE REGEX "/ft2_main\\.c$")
list(FILTER ft2_CORE EXCLUDE REGEX "/ft2_midi\\.c$")

file(GLOB ft2_GFX "${CMAKE_CURRENT_SOURCE_DIR}/../../src/gfxdata/*.c")
file(GLOB ft2_MIX "${CMAKE_CURRENT_SOURCE_DIR}/../../src/mixer/*.c")
file(GLOB ft2_SCOPES "${CMAKE_CURRENT_SOURCE_DIR}/../../src/scopes/*.c")
# compiled through ../src/ft2_scopes_bridge.c instead
list(FILTER ft2_SCOPES EXCLUDE REGEX "/ft2_scopes\\.c$")
file(GLOB ft2_MODLOADERS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/modloaders/*.c")
file(GLOB ft2_SMPLLOADERS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/smploaders/*.c")

set(ft2_SRC
    ${ft2_CORE}
    ${ft2_GFX}
    ${ft2_MIX}
    ${ft2_SCOPES}
    ${ft2_MODLOADERS}
    ${ft2_SMPLLOADERS}
    "../src/ft2_web_platform.c"
    "../src/ft2_wrap_web.c"
    "../src/ft2_mouse_web.c"
    "../src/ft2_main_bridge.c"
    "../src/ft2_scopes_bridge.c"
    "../src/fts_stub.c"
)

# Create executable
add_executable(ft2-wasm ${ft2_SRC})

# Include directories
target_include_directories(ft2-wasm PRIVATE
    "../../src"
    "../src"
    "../include"
)

# Define WASM-specific macros
target_compile_definitions(ft2-wasm PRIVATE
    WASM_BUILD
)

set_target_properties(ft2-wasm PROPERTIES
    C_STANDARD 99
    SUFFIX ".js"
)

target_compile_options(ft2-wasm PRIVATE -O3 -Wno-incompatible-pointer-types)

# Functions marked EMSCRIPTEN_KEEPALIVE in ../src are exported automatically.
target_link_options(ft2-wasm PRIVATE
    -O3
    --closure 1
    --minify 0
    -sMODULARIZE=1
    -sEXPORT_NAME=createFT2Module
    -sEXPORTED_FUNCTIONS=_malloc,_free
    -sEXPORTED_RUNTIME_METHODS=ccall,FS
    -sINVOKE_RUN=0
    -sALLOW_MEMORY_GROWTH=1
    -sSTACK_SIZE=1048576
    -sASSERTIONS=0
    # Asyncify lets FT2's own blocking dialog loops yield to the browser
    -sASYNCIFY=1
    -sASYNCIFY_STACK_SIZE=65536
    # IndexedDB-backed storage for the config file
    -lidbfs.js
    # Browser adaptations of libc/SDL calls, see ../src/ft2_wrap_web.c
    -Wl,--wrap=fopen,--wrap=fclose,--wrap=SDL_RenderPresent,--wrap=usleep
    -Wl,--wrap=SDL_CreateThread,--wrap=SDL_DetachThread
)

# Install target
install(TARGETS ft2-wasm
    RUNTIME DESTINATION web)

#include "ft2_web_platform.h"
#include "ft2_main_bridge.h"
#include "ft2_scopes_bridge.h"

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <stdio.h>
#include <string.h>

#include "../../src/ft2_header.h"
#include "../../src/ft2_audio.h"
#include "../../src/ft2_bmp.h"
#include "../../src/ft2_config.h"
#include "../../src/ft2_diskop.h"
#include "../../src/ft2_events.h"
#include "../../src/ft2_gui.h"
#include "../../src/ft2_hpc.h"
#include "../../src/ft2_module_loader.h"
#include "../../src/ft2_replayer.h"
#include "../../src/ft2_structs.h"
#include "../../src/ft2_video.h"
#include "../../src/mixer/ft2_windowed_sinc.h"
#include "../../src/scopes/ft2_scopes.h"

static bool g_ft2_initialized = false;
static bool g_gui_initialized = false;

/* Set while FT2 code that may open a blocking dialog is on the (possibly
** Asyncify-suspended) stack. While suspended, JavaScript must not start
** another such call, so the exported entry points bail out when this is set.
*/
static bool g_busy = false;
static bool g_nested_loop = false;

bool wasm_in_nested_loop(void)
{
    return g_nested_loop;
}

static void show_error(const char *message)
{
    fprintf(stderr, "[FT2][ERR] %s\n", message);
}

// ---------------------------------------------------------------------------
// Core initialization using original FT2 code
// ---------------------------------------------------------------------------

static bool initialize_runtime_dependencies(void) {
    if (!setupExecutablePath()) {
        show_error("setupExecutablePath() failed");
        return false;
    }

    if (!loadBMPs()) {
        show_error("loadBMPs() failed");
        return false;
    }

    if (!setupWindowedSincTables()) {
        show_error("setupWindowedSincTables() failed");
        return false;
    }

    loadConfigOrSetDefaults();

    hpc_Init();
    hpc_SetDurationInHz(&video.vblankHpc, VBLANK_HZ);

    if (!setupWindow()) {
        show_error("setupWindow() failed");
        return false;
    }

    if (!setupRenderer()) {
        show_error("setupRenderer() failed");
        return false;
    }

    // always use FT2's own 60Hz frame wait, it is what makes nested loops yield
    video.vsync60HzPresent = false;

    if (!setupDiskOp()) {
        show_error("setupDiskOp() failed");
        return false;
    }

    // Setup audio device - this is critical for playback!
    if (!setupAudio(false)) {
        show_error("setupAudio() failed - audio may not work");
        // Don't fail completely, allow operation without audio
    }

    if (!setupReplayer()) {
        show_error("setupReplayer() failed");
        return false;
    }

    updateChanNums();

    editor.programRunning = true;
    editor.mainLoopOngoing = true;

    hpc_ResetCounters(&video.vblankHpc);
    return true;
}

// Asynchronous (waits for IndexedDB): call with ccall(..., {async: true})
EMSCRIPTEN_KEEPALIVE bool ft2_init_web(void) {
    if (g_ft2_initialized)
        return true;

    // must come before FT2 looks for its config file
    wasm_mount_storage();

    SDL_SetHint("SDL_EMSCRIPTEN_CANVAS_SELECTOR", "#ft2-canvas");
    SDL_StopTextInput();

    // Initialize SDL audio subsystem (video is handled separately)
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        show_error("SDL_InitSubSystem(SDL_INIT_AUDIO) failed");
        // Don't fail - allow operation without audio
    }

    wasm_initialize_vars();
    setupCrashHandler();

    if (!initialize_runtime_dependencies())
        return false;

    if (!initScopes()) {
        show_error("initScopes() failed");
        return false;
    }

    g_ft2_initialized = true;
    return true;
}

EMSCRIPTEN_KEEPALIVE bool ft2_init_full_ui(void) {
    if (!g_ft2_initialized) {
        show_error("Call ft2_init_web() before ft2_init_full_ui()");
        return false;
    }

    if (g_gui_initialized)
        return true;

    if (!setupGUI()) {
        show_error("setupGUI() failed");
        return false;
    }

    // as in ft2_main.c: restart the audio stream so its clock is in sync with the UI
    pauseAudio();
    resumeAudio();

    drawGUIOnRunTime();
    flipFrame();

    g_gui_initialized = true;
    return true;
}

// ---------------------------------------------------------------------------
// Main loop body (one iteration of the loop in ft2_main.c)
// ---------------------------------------------------------------------------

/* Call once per requestAnimationFrame. May suspend (Asyncify) while a dialog
** is open; calls made in the meantime return immediately.
*/
EMSCRIPTEN_KEEPALIVE void ft2_render_frame(void) {
    static double nextFrameTime = 0.0;

    if (!g_gui_initialized || !editor.programRunning || g_busy)
        return;

    // FT2 logic is tied to 60Hz, don't run faster on high refresh rate displays
    const double frameTime = 1000.0 / VBLANK_HZ;
    const double now = emscripten_get_now();
    if (now < nextFrameTime - 2.0)
        return;

    nextFrameTime += frameTime;
    if (nextFrameTime < now)
        nextFrameTime = now + frameTime;

    g_busy = true;

    beginFPSCounter();

    g_nested_loop = true;
    handleThreadEvents();
    readInput();
    handleEvents();
    wasm_update_scopes();
    handleRedrawing();
    g_nested_loop = false;

    flipFrame();
    endFPSCounter();

    g_busy = false;
}

// ---------------------------------------------------------------------------
// Exported helpers for JavaScript
// ---------------------------------------------------------------------------

/* Loads a module, instrument or sample from the virtual filesystem, exactly
** like a file dropped onto the native program (modules start playing).
** FT2 reports errors in its own dialogs. Asynchronous if a dialog is shown:
** call with ccall(..., {async: true}). Ignored while a dialog is open.
*/
EMSCRIPTEN_KEEPALIVE void ft2_load_file(const char *filename) {
    if (filename == NULL || !g_gui_initialized || g_busy)
        return;

    char path[PATH_MAX];
    strncpy(path, filename, PATH_MAX - 1);
    path[PATH_MAX - 1] = '\0';

    g_busy = true;
    g_nested_loop = true;

    editor.autoPlayOnDrop = true;
    loadDroppedFile(path);

    g_nested_loop = false;
    g_busy = false;
}

EMSCRIPTEN_KEEPALIVE bool ft2_is_dialog_open(void) {
    return ui.sysReqShown;
}

EMSCRIPTEN_KEEPALIVE int32_t ft2_get_position_song_pos(void) {
    return editor.songPos;
}

EMSCRIPTEN_KEEPALIVE int32_t ft2_get_position_pattern(void) {
    return editor.editPattern;
}

EMSCRIPTEN_KEEPALIVE int32_t ft2_get_position_row(void) {
    return editor.row;
}

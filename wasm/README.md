# FastTracker 2 Clone - WebAssembly Port

This directory contains the WebAssembly port of the FastTracker 2 clone. The original C sources in `../src` are compiled as-is with Emscripten; the FT2 interface is drawn on a 632x400 canvas.

- [EMBEDDING.md](EMBEDDING.md): how to put FT2 on your own page
- [DEPLOYMENT.md](DEPLOYMENT.md): hosting notes

## Build

Requirements: Emscripten 5.0.2 (the version the build is tested with) and CMake. If `emcc` is not on the PATH, the build script installs that Emscripten version into `wasm/build/emsdk`.

```bash
cd wasm/build
./build-wasm.sh
```

Output goes to `wasm/web`. To run it:

```bash
cd wasm/web
python3 -m http.server 8000
# open http://localhost:8000/index.html
```

## Deployable files

Exactly four files, all in `wasm/web`:

| File | Purpose |
|------|---------|
| `index.html` | Page, startup code, mouse forwarding, module loading UI |
| `ft2-clone.css` | Page styles |
| `ft2-wasm.js` | Emscripten loader (defines `createFT2Module`) |
| `ft2-wasm.wasm` | Compiled FT2 clone, about 1.9 MB uncompressed |

Nothing else is required. There is no JavaScript wrapper class; a page uses the Emscripten module directly, as `index.html` does.

## Behaviour in the browser

- **Saving**: every file FT2 writes (modules, instruments, samples, patterns, tracks, WAV renders) is offered as a browser download when FT2 closes it.
- **Settings**: "Save config" is persisted in IndexedDB (IDBFS mounted at `/home/web_user/.config`) and restored on the next visit.
- **Dialogs**: FT2's own system requests, drawn on the canvas. No browser `alert`/`confirm`/`prompt`.
- **Audio**: SDL2's Emscripten backend, which uses a ScriptProcessorNode on the main thread. No AudioWorklet and no SharedArrayBuffer, so no COOP/COEP headers are needed. Browsers require a user gesture before audio starts. Heavy UI work can cause audio glitches.
- **Keyboard**: handled by SDL2, which listens on the window.
- **Mouse**: forwarded by the page to exported functions (see below).

## Known limitations

- No MIDI.
- No touch input.
- No threads: long operations (WAV render, resample, etc.) block the page until done.
- Sample recording from an input device is untested.
- The sample modules listed in `index.html` are fetched from api.modarchive.org and modland.com and depend on those hosts allowing cross-origin requests.

## JavaScript API

`ft2-wasm.js` defines a global factory:

```javascript
const Module = await createFT2Module({ canvas, print, printErr });
```

The canvas must have id `ft2-canvas` and be 632x400 (CSS may scale it).

The build uses Asyncify. Functions marked *async* below must be called through `ccall` with `{ async: true }` and awaited; calling them as plain `Module._name()` returns before they finish.

### Startup

```javascript
await Module.ccall('ft2_init_web', 'boolean', [], [], { async: true });
await Module.ccall('ft2_init_full_ui', 'boolean', [], [], { async: true });

function frame() {
    Module._ft2_render_frame();
    requestAnimationFrame(frame);
}
frame();
```

| Function | Notes |
|----------|-------|
| `ft2_init_web()` | *async*. Mounts storage and initialises FT2's core. Returns true on success. |
| `ft2_init_full_ui()` | *async*. Sets up and draws the GUI. Must follow `ft2_init_web`. Returns true on success. |
| `_ft2_render_frame()` | Call once per `requestAnimationFrame`. Limits itself to 60 Hz and returns immediately while a previous call is suspended in a dialog. |

### Loading a module

```javascript
Module.FS.writeFile(path, uint8Array);
await Module.ccall('ft2_load_file', null, ['string'], [path], { async: true });
```

*async*. Behaves like a file dropped onto the native program: loads a module (and starts playback), instrument or sample, asks before replacing an unsaved song, and reports errors in FT2's own dialogs. Returns nothing. Does nothing if a dialog is already open.

### Mouse

x/y are in 632x400 FT2 screen coordinates; `button` and `buttons` are as in a DOM `MouseEvent`.

- `_ft2_mouse_move(x, y, buttons)`
- `_ft2_mouse_button_down(button, x, y, buttons)`
- `_ft2_mouse_button_up(button, x, y, buttons)`
- `_ft2_mouse_wheel(deltaY)`

### Read-only state

- `_ft2_get_position_song_pos()`
- `_ft2_get_position_pattern()`
- `_ft2_get_position_row()`
- `_ft2_is_dialog_open()`

### Runtime methods

Only `ccall` and `FS` are exported. The JavaScript is built with the Closure compiler, so only the FS methods the page uses are reliably available: `writeFile`, `readFile`, `mkdir`, `unlink`.

## Architecture

The original sources in `../src` are compiled as they are, except:

- `ft2_main.c` is not compiled directly; it is included by `ft2_main_bridge.c` with its `main` renamed.
- `scopes/ft2_scopes.c` is included by `ft2_scopes_bridge.c`, which leaves out the scope thread.
- `ft2_midi.c` is excluded.
- Four files carry small `__EMSCRIPTEN__`/`WASM_BUILD` guards: `ft2_mouse.c`, `ft2_video.c`, `ft2_sysreqs.c` and `ft2_unicode.c`.

Platform glue lives in `wasm/src`:

| File | Role |
|------|------|
| `ft2_web_platform.c` | Init, per-frame main loop body, functions exported to JavaScript |
| `ft2_wrap_web.c` | Link-time wrappers (`--wrap`) for `fopen`/`fclose` (downloads, config persistence), `SDL_RenderPresent`/`usleep` (yielding to the browser) and `SDL_CreateThread` (worker "threads" run synchronously) |
| `ft2_mouse_web.c` | Mouse functions exported to JavaScript |
| `ft2_main_bridge.c` | Includes `ft2_main.c` without its entry point |
| `ft2_scopes_bridge.c` | Includes `ft2_scopes.c` without its thread; scopes are updated once per frame |
| `fts_stub.c` | Stub for the `fts_*` directory functions |

Build configuration is in `wasm/build/CMakeLists.txt`.

## Tests

```bash
cd wasm/tests
npm install
npx playwright install chromium
npm test
```

This runs a headless smoke test against `wasm/web`: boot, load and play a module, dialogs, save download, config persistence. Set `FT2_BROWSER=chrome` to use an installed Chrome instead of Playwright's Chromium.

## License

Same as the FastTracker 2 clone; see `../LICENSE`.

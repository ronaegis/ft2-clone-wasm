# Embedding the FT2 WASM Port

There is no JavaScript wrapper library. Embedding means loading `ft2-wasm.js` and driving the Emscripten module directly, as `web/index.html` does. See [README.md](README.md#javascript-api) for the full function list.

## Required files

Place `ft2-wasm.js` and `ft2-wasm.wasm` next to your page. (`index.html` and `ft2-clone.css` are only needed if you use the stock page.)

## Minimal example

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>FT2</title>
</head>
<body>
    <canvas id="ft2-canvas" width="632" height="400" style="image-rendering: pixelated;"></canvas>
    <input type="file" id="module-file">

    <script src="ft2-wasm.js"></script>
    <script>
    (async function () {
        const canvas = document.getElementById('ft2-canvas');

        const Module = await createFT2Module({
            canvas: canvas,
            print: (text) => console.log(text),
            printErr: (text) => console.error(text)
        });

        // The build uses Asyncify: these must go through ccall with { async: true }
        if (!await Module.ccall('ft2_init_web', 'boolean', [], [], { async: true }))
            throw new Error('ft2_init_web failed');
        if (!await Module.ccall('ft2_init_full_ui', 'boolean', [], [], { async: true }))
            throw new Error('ft2_init_full_ui failed');

        // Once per frame
        function frame() {
            Module._ft2_render_frame();
            requestAnimationFrame(frame);
        }
        frame();

        // Mouse: convert to 632x400 FT2 screen coordinates
        function pos(e) {
            const r = canvas.getBoundingClientRect();
            return {
                x: Math.floor((e.clientX - r.left) * (632 / r.width)),
                y: Math.floor((e.clientY - r.top) * (400 / r.height))
            };
        }
        canvas.addEventListener('mousemove', (e) => {
            const p = pos(e);
            Module._ft2_mouse_move(p.x, p.y, e.buttons);
        });
        canvas.addEventListener('mousedown', (e) => {
            e.preventDefault();
            const p = pos(e);
            Module._ft2_mouse_button_down(e.button, p.x, p.y, e.buttons);
        });
        canvas.addEventListener('mouseup', (e) => {
            e.preventDefault();
            const p = pos(e);
            Module._ft2_mouse_button_up(e.button, p.x, p.y, e.buttons);
        });
        canvas.addEventListener('wheel', (e) => {
            e.preventDefault();
            Module._ft2_mouse_wheel(e.deltaY);
        });
        canvas.addEventListener('contextmenu', (e) => e.preventDefault());

        // Load a file picked by the user (modules start playing)
        async function loadModule(name, bytes) {
            const path = '/' + name;
            Module.FS.writeFile(path, bytes);
            await Module.ccall('ft2_load_file', null, ['string'], [path], { async: true });
            Module.FS.unlink(path);
        }
        document.getElementById('module-file').addEventListener('change', async (e) => {
            const file = e.target.files[0];
            if (file)
                await loadModule(file.name, new Uint8Array(await file.arrayBuffer()));
        });
    })();
    </script>
</body>
</html>
```

To load a module from a URL, fetch it and pass the bytes to the same function:

```javascript
const response = await fetch(url);
await loadModule('song.xm', new Uint8Array(await response.arrayBuffer()));
```

## Things to know

- **Canvas**: the id must be `ft2-canvas` and the size 632x400. Scale it with CSS if needed; mouse coordinates must then be converted as above.
- **Asyncify**: `ft2_init_web`, `ft2_init_full_ui` and `ft2_load_file` must be called through `ccall` with `{ async: true }` and awaited. Called as plain `Module._name()` they return before they finish.
- **Render loop**: `_ft2_render_frame()` limits itself to 60 Hz and returns immediately while a previous call is suspended in a dialog, so calling it on every `requestAnimationFrame` is safe.
- **Loading**: `ft2_load_file` behaves like a file dropped onto the native program: it accepts modules, instruments and samples, asks before replacing an unsaved song, and reports errors in FT2's own dialogs on the canvas. It returns nothing, and does nothing if a dialog is already open; `Module._ft2_is_dialog_open()` tells you.
- **Keyboard**: handled by SDL2, which listens on the window. No page code is needed.
- **Mouse release outside the canvas**: `index.html` additionally sends `_ft2_mouse_button_up` on window `mouseup` and `blur` so buttons do not stay stuck.
- **Audio**: starts only after a user gesture (browser policy).
- **Saving**: files FT2 writes are offered as browser downloads; settings are kept in IndexedDB. No page code is needed.
- **FS**: only `writeFile`, `readFile`, `mkdir` and `unlink` are reliably available on `Module.FS` (Closure-compiled build).
- **Cross-origin modules**: fetching a module from another host requires that host to allow cross-origin requests, and your Content-Security-Policy `connect-src` (if any) to allow it.
- **Playback position**: `_ft2_get_position_song_pos()`, `_ft2_get_position_pattern()`, `_ft2_get_position_row()`.

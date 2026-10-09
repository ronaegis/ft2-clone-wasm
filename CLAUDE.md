minimize changes to the original source code, make all the stubs in wasm folder

Re-use/compile/link as much original code as possible

Do not COPY existing code, use/compile/link the existing code, by either including or creating stub functions.

Mininize re-implementing things, reuse/compile/link in what is already there

YOU MUST use the original FT2 code, not reimplement it!

Try to not commit modifications to the original FT2 clone files, you can temporarily add debug statements in them in order to debug issues.

The built web/wasm files are in "wasm/web". they can be served with "python3 -m http.server 8000"

The page to load the FT2 clone is "wasm/web/index.html"

To build, run "./build-wasm.sh" in "wasm/build"

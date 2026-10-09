# ft2-clone - WebAssembly port

**Try it in your browser: https://ronaegis.github.io/ft2-clone-wasm/**

This is a fork of [8bitbubsy/ft2-clone](https://github.com/8bitbubsy/ft2-clone) that builds the Fasttracker II clone for the web with Emscripten. The tracker itself is the upstream code; this fork adds what it needs to run on a web page.

## What it does to the upstream code

The upstream sources in `src/` are compiled as they are. The port lives in `wasm/` and adapts them from the outside:

- **Link-time wrappers** (`wasm/src/ft2_wrap_web.c`) replace a few libc/SDL calls without editing the callers:
  - `fopen`/`fclose`: every file FT2 saves is offered as a browser download, and the config is kept in IndexedDB.
  - `SDL_CreateThread`: there are no threads, so FT2's worker threads run to completion on the main thread.
  - `SDL_RenderPresent`/`usleep`: FT2's own blocking loops (its dialogs) yield to the browser each frame, using Asyncify.
- **Two bridge files** include upstream files to reach what they keep private: `ft2_main.c` without its `main()`, and `scopes/ft2_scopes.c` without its scope thread (scopes are updated once per frame instead).
- **Four upstream files carry small guards** (89 lines in total):
  - `ft2_mouse.c`: the mouse position comes from the page, not from SDL.
  - `ft2_video.c`: no high-DPI window.
  - `ft2_sysreqs.c`: thread-safe message boxes are shown directly.
  - `ft2_unicode.c`: paths are passed through, since Emscripten's libc has no CP850 conversion.
- **Not included:** MIDI (`ft2_midi.c` is left out), touch input, and real threads, so long operations such as WAV rendering block the page until they finish.

Everything else is new and separate from upstream: the web page (`wasm/web`), a headless smoke test (`wasm/tests`), and a CI workflow that builds, tests and deploys the page. See [wasm/README.md](wasm/README.md) for building and embedding.

---

The upstream README follows.

# ft2-clone
Fasttracker II clone for Windows/macOS/Linux

Aims to be a highly accurate clone of the classic Fasttracker II software for MS-DOS. \
The XM player itself has been directly ported from the original source code, for maximum accuracy. \
The code is partly my own, partly based on the original FT2 code.

*What is Fasttracker II? Read about it on [Wikipedia](https://en.wikipedia.org/wiki/FastTracker_2).*

# Releases
Windows/macOS binary releases can always be found at [16-bits.org](https://16-bits.org/ft2.php).

Linux binaries can be found [here](https://repology.org/project/fasttracker2/versions). \
If these don't work for you, you'll have to compile the code manually.

# Improvements over original DOS version
- New sample editor features, like waveform generators and resonant filters
- The channel resampler/mixer uses floating-point arithmetics for less errors, and has extra interpolation options (4-point cubic spline and 8-point/16-point windowed-sinc)
- The sample loader supports AIFF/FLAC/OGG/MP3/BRR (SNES) samples and more WAV types than original FT2. It will also attempt to tune the sample (finetune and rel. note) to its playback frequency on load.
- It contains a new "Trim" feature, which will remove unused stuff to potentially make the module smaller
- Drag n' drop of modules/samples
- The waveform display in the sample editor shows peak based data when zoomed out
- Textboxes have a text marking option, where you can cut/copy/paste
- MOD/STM/S3M import has been slightly improved (S3M import is still not ideal, as it's not compatible with XM)
- Supports loading DIGI Booster (non-Pro) modules
- Supports loading Impulse Tracker modules (Awful support! Don't use this for playback)
- It supports loading XMs with stereo samples, uneven amount of channels, more than 32 channels, more than 16 samples per instrument, more than 128 patterns etc. The unsupported data will be mixed to mono/truncated.
- It has some small additions to make life easier (C4/middle-C Hz display in Instr. Ed., envelope point coordinate display, etc).

# Screenshots

![Example #1](https://16-bits.org/ft2-clone-3.png)
![Example #2](https://16-bits.org/ft2-clone-4.png)

# Compiling the code
Build instructions can be found in the repository (HOW-TO-COMPILE.txt).

Keep in mind that the program may fail to compile on Linux, depending on your distribution and GCC version. \
Please don't nag me about it, and try to use the Linux packages linked to from [16-bits.org](https://16-bits.org/ft2.php) instead.

PS: The source code is quite hackish and hardcoded. \
My first priority is to make an accurate clone, and not to make flexible and easily modifiable code.

Big parts of the code (except GUI) are directly ported from the original FT2 source code, with permission to use a BSD 3-Clause license.

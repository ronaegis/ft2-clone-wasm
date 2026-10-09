/* Link-time wrappers (-Wl,--wrap=...) that adapt the unmodified FT2 sources
** to the browser, so that the original code needs no WASM-specific edits:
**
** - fopen/fclose:        every file FT2 saves is offered as a browser download,
**                        and config files are persisted to IndexedDB.
** - SDL_RenderPresent:   FT2's own blocking loops (system requests etc.) yield
**                        to the browser once per frame (needs Asyncify).
** - usleep:              the 60Hz frame wait must not busy-wait the main thread.
** - SDL_CreateThread:    there are no threads; FT2's worker "threads" run to
**                        completion on the main thread.
*/

#include <emscripten.h>
#include <SDL2/SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ft2_web_platform.h"

// FT2 puts its config files in "$HOME/.config/FT2 clone" (see ft2_config.c)
#define PERSIST_DIR "/home/web_user/.config"

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

EM_ASYNC_JS(void, wasm_mount_storage_js, (const char *dir), {
	var d = UTF8ToString(dir);
	try {
		FS.mkdirTree(d);
		FS.mount(IDBFS, {}, d);
		await new Promise(function(resolve) {
			FS.syncfs(true, function(err) {
				if (err) console.warn('FT2: could not read saved settings:', err);
				resolve();
			});
		});
	} catch (e) {
		console.warn('FT2: persistent storage unavailable, settings will not be kept:', e);
	}
});

// One save can close several config files: coalesce into a single sync
EM_JS(void, wasm_persist_storage, (void), {
	var state = Module['ft2Sync'] || (Module['ft2Sync'] = { busy: false, dirty: false });
	if (state.busy) {
		state.dirty = true;
		return;
	}

	state.busy = true;
	setTimeout(function run() {
		state.dirty = false;
		FS.syncfs(false, function(err) {
			if (err) console.warn('FT2: could not save settings:', err);
			if (state.dirty) run(); else state.busy = false;
		});
	}, 0);
});

EM_JS(void, wasm_trigger_download, (const char *path), {
	var p = UTF8ToString(path);
	try {
		var blob = new Blob([FS.readFile(p)], { type: 'application/octet-stream' });
		var link = document.createElement('a');
		link.href = URL.createObjectURL(blob);
		link.download = p.split('/').pop();
		document.body.appendChild(link);
		link.click();
		document.body.removeChild(link);
		setTimeout(function() { URL.revokeObjectURL(link.href); }, 10000);
	} catch (e) {
		console.error('FT2: could not download ' + p + ':', e);
	}
});

void wasm_mount_storage(void)
{
	wasm_mount_storage_js(PERSIST_DIR);
}

// ---------------------------------------------------------------------------
// fopen/fclose: track files opened for writing
// ---------------------------------------------------------------------------

#define MAX_WRITTEN_FILES 8

static struct
{
	FILE *f;
	char path[PATH_MAX];
} writtenFiles[MAX_WRITTEN_FILES];

FILE *__real_fopen(const char *path, const char *mode);
int __real_fclose(FILE *f);

FILE *__wrap_fopen(const char *path, const char *mode)
{
	FILE *f = __real_fopen(path, mode);
	if (f == NULL || strchr(mode, 'w') == NULL)
		return f;

	for (int i = 0; i < MAX_WRITTEN_FILES; i++)
	{
		if (writtenFiles[i].f != NULL)
			continue;

		// resolve now, FT2 may change directory before the file is closed
		if (realpath(path, writtenFiles[i].path) != NULL)
			writtenFiles[i].f = f;

		break;
	}

	return f;
}

int __wrap_fclose(FILE *f)
{
	int slot = -1;
	for (int i = 0; i < MAX_WRITTEN_FILES; i++)
	{
		if (f != NULL && writtenFiles[i].f == f)
		{
			slot = i;
			break;
		}
	}

	const int result = __real_fclose(f);
	if (slot < 0)
		return result;

	writtenFiles[slot].f = NULL;
	if (result == 0)
	{
		const char *path = writtenFiles[slot].path;
		if (strncmp(path, PERSIST_DIR "/", sizeof (PERSIST_DIR)) == 0)
			wasm_persist_storage();
		else
			wasm_trigger_download(path);
	}

	return result;
}

// ---------------------------------------------------------------------------
// Frame pacing
// ---------------------------------------------------------------------------

void __real_SDL_RenderPresent(SDL_Renderer *renderer);

void __wrap_SDL_RenderPresent(SDL_Renderer *renderer)
{
	__real_SDL_RenderPresent(renderer);

	// let the browser deliver input, run audio and paint between loop iterations
	if (wasm_in_nested_loop())
		emscripten_sleep(0);
}

int __wrap_usleep(useconds_t usec)
{
	/* Emscripten's usleep() busy-waits. The top-level frame is already paced
	** by requestAnimationFrame, so only FT2's nested loops need a real wait.
	*/
	if (wasm_in_nested_loop())
		emscripten_sleep(usec / 1000);

	return 0;
}

// ---------------------------------------------------------------------------
// Threads
// ---------------------------------------------------------------------------

SDL_Thread *__wrap_SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data)
{
	(void)name;

	/* FT2 only uses detached worker threads that report back through flags
	** polled by the main loop, so running them synchronously is equivalent.
	*/
	fn(data);

	static int dummyThread;
	return (SDL_Thread *)&dummyThread; // non-NULL = success
}

void __wrap_SDL_DetachThread(SDL_Thread *thread)
{
	(void)thread;
}

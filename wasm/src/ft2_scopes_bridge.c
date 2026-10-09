#include <SDL2/SDL.h>
#include "ft2_scopes_bridge.h"

/* The scope thread loops forever, which cannot work without real threads.
** Don't start it; scopes are updated once per frame from the main loop instead.
*/
#define SDL_CreateThread(fn, name, data) ((void)(fn), (SDL_Thread *)1)

#include "../../src/scopes/ft2_scopes.c"

void wasm_update_scopes(void)
{
	updateScopes();
}

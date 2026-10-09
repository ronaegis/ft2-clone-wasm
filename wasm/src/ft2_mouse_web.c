#include <emscripten.h>
#include <SDL2/SDL.h>
#include <string.h>

#include "../../src/ft2_header.h"
#include "../../src/ft2_mouse.h"

extern mouse_t mouse;

static uint8_t js_button_to_sdl(int button)
{
    switch (button)
    {
        case 0: return SDL_BUTTON_LEFT;
        case 1: return SDL_BUTTON_MIDDLE;
        case 2: return SDL_BUTTON_RIGHT;
        default: return (uint8_t)(button + 1);
    }
}

static uint32_t dom_buttons_to_sdl_mask(int buttons)
{
    /* DOM "buttons" is a bitfield: primary=1, secondary=2, auxiliary=4, etc. */
    uint32_t mask = 0;
    if (buttons & 1) mask |= SDL_BUTTON_LMASK;
    if (buttons & 2) mask |= SDL_BUTTON_RMASK;
    if (buttons & 4) mask |= SDL_BUTTON_MMASK;
    if (buttons & 8) mask |= SDL_BUTTON_X1MASK;
    if (buttons & 16) mask |= SDL_BUTTON_X2MASK;
    return mask;
}

static void update_mouse_state(int x, int y, int buttons, int *xRel, int *yRel)
{
    if (xRel != NULL) *xRel = x - mouse.x;
    if (yRel != NULL) *yRel = y - mouse.y;

    mouse.lastX = mouse.x;
    mouse.lastY = mouse.y;
    mouse.x = x;
    mouse.y = y;
    mouse.rawX = x;
    mouse.rawY = y;
    mouse.absX = x;
    mouse.absY = y;
    mouse.buttonState = dom_buttons_to_sdl_mask(buttons);
}

EMSCRIPTEN_KEEPALIVE void ft2_mouse_button_down(int button, int x, int y, int buttons)
{
    int xRel, yRel;
    update_mouse_state(x, y, buttons, &xRel, &yRel);

    SDL_Event event;
    memset(&event, 0, sizeof (event));
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = js_button_to_sdl(button);
    event.button.state = SDL_PRESSED;
    event.button.clicks = 1;
    event.button.x = x;
    event.button.y = y;
    SDL_PushEvent(&event);
}

EMSCRIPTEN_KEEPALIVE void ft2_mouse_button_up(int button, int x, int y, int buttons)
{
    int xRel, yRel;
    update_mouse_state(x, y, buttons, &xRel, &yRel);

    SDL_Event event;
    memset(&event, 0, sizeof (event));
    event.type = SDL_MOUSEBUTTONUP;
    event.button.button = js_button_to_sdl(button);
    event.button.state = SDL_RELEASED;
    event.button.clicks = 1;
    event.button.x = x;
    event.button.y = y;
    SDL_PushEvent(&event);
}

EMSCRIPTEN_KEEPALIVE void ft2_mouse_move(int x, int y, int buttons)
{
    int xRel, yRel;
    update_mouse_state(x, y, buttons, &xRel, &yRel);

    SDL_Event event;
    memset(&event, 0, sizeof (event));
    event.type = SDL_MOUSEMOTION;
    event.motion.state = dom_buttons_to_sdl_mask(buttons);
    event.motion.x = x;
    event.motion.y = y;
    event.motion.xrel = xRel;
    event.motion.yrel = yRel;
    SDL_PushEvent(&event);
}

EMSCRIPTEN_KEEPALIVE void ft2_mouse_wheel(int delta)
{
    mouseWheelHandler(delta > 0);
}

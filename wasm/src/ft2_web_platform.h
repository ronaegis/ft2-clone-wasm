#pragma once

#include <stdint.h>
#include <stdbool.h>

// Functions exported to JavaScript (see wasm/web/index.html)
bool ft2_init_web(void);
bool ft2_init_full_ui(void);
void ft2_render_frame(void);
void ft2_load_file(const char *filename);
int32_t ft2_get_position_song_pos(void);
int32_t ft2_get_position_pattern(void);
int32_t ft2_get_position_row(void);
bool ft2_is_dialog_open(void);

/* True while original FT2 code is running one of its own blocking loops
** (system requests, sample editor dialogs). Such loops must yield to the
** browser on every frame, see ft2_wrap_web.c.
*/
bool wasm_in_nested_loop(void);

// Browser-side storage (ft2_wrap_web.c)
void wasm_mount_storage(void);

#ifndef GXM_H
#define GXM_H

int gxm_init(void);

int gxm_fini(void);

void gxm_draw_start(void);

void gxm_draw_end(void);

void gxm_clear_screen(void);

void gxm_swap_screen(void);

void gxm_update(void);

void render_message_dialog(void *color, void *depth, void *sync);

#endif
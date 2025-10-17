#ifndef HG_DEPTH_RENDERER_H
#define HG_DEPTH_RENDERER_H

#include "hg_graphics.h"

void hg_depth_renderer_init(void);
void hg_depth_renderer_shutdown(void);

void hg_depth_renderer_target_create(u32 width, u32 height, HgTexture** target, HgTexture** depth_buffer);
void hg_depth_render_draw(HgTexture* target, HgTexture* depth_buffer);

#endif // HG_DEPTH_RENDERER_H

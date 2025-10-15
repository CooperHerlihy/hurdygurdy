#include "hg_2d_renderer.h"

#include "hg_graphics.h"
#include "hg_math.h"

HgTexture* hg_2d_renderer_g_target;
HgTexture* hg_2d_renderer_g_depth_buffer;

void hg_2d_renderer_init(u32 width, u32 height);

void hg_2d_renderer_update_size(u32 width, u32 height);

void hg_2d_renderer_shutdown(void);

HgTexture* hg_sprite_create(const u32* data, u32 width, u32 height, HgFormat format);

void hg_sprite_destroy(HgTexture* texture);

void hg_2d_renderer_camera_move(HgVec2 delta);

void hg_2d_renderer_queue_rect(HgVec4 color, HgTransform2D transform);

void hg_2d_renderer_queue_sprite(HgTexture* texture, HgVec2 offset, HgVec2 size, HgTransform2D transform);

void hg_2d_renderer_draw(void);


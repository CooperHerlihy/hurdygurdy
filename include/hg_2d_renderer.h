#ifndef HG_2D_RENDERER_H
#define HG_2D_RENDERER_H

#include "hg_math.h"
#include "hg_graphics.h"

typedef struct HgTransform2D {
    HgVec3 position;
    HgVec2 scale;
    f32 rotation;
} HgTransform2D;

extern HgTexture* hg_2d_renderer_g_target;
extern HgTexture* hg_2d_renderer_g_depth_buffer;

void hg_2d_renderer_init(u32 width, u32 height);
void hg_2d_renderer_update_size(u32 width, u32 height);
void hg_2d_renderer_shutdown(void);

HgTexture* hg_sprite_create(const u32* data, u32 width, u32 height, HgFormat format, bool filter);
void hg_sprite_destroy(HgTexture* texture);

void hg_2d_renderer_camera_position(HgVec2 position);
void hg_2d_renderer_camera_move(HgVec2 delta);

void hg_2d_renderer_queue_sprite(HgTexture* texture, HgVec2 offset, HgVec2 extent, HgTransform2D transform);

void hg_2d_renderer_draw(void);

#endif // HG_2D_RENDERER_H

#ifndef HG_2D_RENDERER_H
#define HG_2D_RENDERER_H

#include "hg_math.h"
#include "hg_graphics.h"

typedef struct HgTransform2D {
    HgVec3 position;
    HgVec2 scale;
    f32 rotation;
} HgTransform2D;

void hg_2d_renderer_init(void);
void hg_2d_renderer_shutdown(void);

void hg_2d_renderer_target_create(u32 width, u32 height, HgTexture** target, HgTexture** depth_buffer);

HgTexture* hg_2d_sprite_create(const u32* data, u32 width, u32 height, HgFormat format, bool filter);

void hg_2d_renderer_update_projection(f32 aspect, f32 near, f32 far);
void hg_2d_renderer_update_view(HgVec3 position, f32 zoom, HgQuat rotation);

void hg_2d_renderer_queue_sprite(HgTexture* texture, HgVec2 offset, HgVec2 extent, HgTransform2D* transform);

void hg_2d_renderer_draw(HgTexture* target, HgTexture* depth_buffer);

#endif // HG_2D_RENDERER_H

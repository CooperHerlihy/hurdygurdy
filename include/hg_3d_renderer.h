#ifndef HG_3D_RENDERER_H
#define HG_3D_RENDERER_H

#include "hg_math.h"
#include "hg_graphics.h"

typedef struct HgTransform3D {
    HgVec3 position;
    HgVec3 scale;
    HgQuat rotation;
} HgTransform3D;

void hg_3d_renderer_init(void);
void hg_3d_renderer_shutdown(void);

void hg_3d_renderer_target_create(u32 width, u32 height, HgTexture** target, HgTexture** depth_buffer);

typedef struct HgModel3D {
    HgBuffer* vertex_buffer;
    HgBuffer* index_buffer;
    HgTexture* color_map;
    HgTexture* normal_map;
} HgModel3D;

typedef struct HgVertex3D {
    HgVec3 position;
    HgVec3 normal;
    HgVec4 tangent;
    HgVec2 uv;
} HgVertex3D;

HgBuffer* hg_3d_vertex_buffer_create(const HgVertex3D* vertices, u32 vertex_count);
HgBuffer* hg_3d_index_buffer_create(const u32* indices, u32 index_count);
HgTexture* hg_3d_texture_map_create(const void* data, u32 width, u32 height, HgFormat format, bool filter);

void hg_3d_renderer_update_projection(f32 fov, f32 aspect, f32 near, f32 far);
void hg_3d_renderer_update_view(HgVec3 position, f32 zoom, HgQuat rotation);

void hg_3d_renderer_set_directional_light(HgVec3 direction, HgVec3 color, f32 intensity);
void hg_3d_renderer_queue_point_light(HgVec3 position, HgVec3 color, f32 intensity);

void hg_3d_renderer_queue_model(HgModel3D* model, HgTransform3D* transform);
void hg_3d_renderer_draw(HgTexture* target, HgTexture* depth_buffer);

#endif // HG_3D_RENDERER_H

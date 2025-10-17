#ifndef HG_RAY_MARCHER_H
#define HG_RAY_MARCHER_H

#include "hg_math.h"
#include "hg_graphics.h"

void hg_ray_marcher_init(void);
void hg_ray_marcher_shutdown(void);

void hg_ray_marcher_draw(HgTexture* target, HgMat4* view, f32 aspect);

#endif // HG_RAY_MARCHER_H

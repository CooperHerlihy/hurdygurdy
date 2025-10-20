#ifndef HURDY_GURDY_H
#define HURDY_GURDY_H

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_graphics.h"
#include "hg_depth_renderer.h"
#include "hg_2d_renderer.h"
#include "hg_3d_renderer.h"
#include "hg_ray_marcher.h"

HgError hg_init(void);
void hg_shutdown(void);

#endif // HURDY_GURDY_H

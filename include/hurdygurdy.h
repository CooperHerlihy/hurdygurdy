#ifndef HURDY_GURDY_H
#define HURDY_GURDY_H

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_graphics.h"
#include "hg_input.h"

/**
 * Initializes all hurdygurdy subsystems
 *
 * @return HG_SUCCESS if the library was initialized successfully
 */
HgError hg_init(void);

/**
 * Shuts down all hurdygurdy subsystems
 *
 * This function can be called in order to reinitialize the library
 */
void hg_shutdown(void);

#endif // HURDY_GURDY_H

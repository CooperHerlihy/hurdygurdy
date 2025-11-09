#ifndef HG_INIT_H
#define HG_INIT_H

#include "hg_enums.h"
#include "hg_utils.h"

/**
 * The context for the HurdyGurdy library
 */
typedef struct HurdyGurdy HurdyGurdy;

/**
 * Initializes HurdyGurdy
 *
 * \return The created HurdyGurdy context, is never NULL
 */
HurdyGurdy* hg_init(void);

/**
 * Shuts down HurdyGurdy
 *
 * \param hg The graphics context, must not be NULL
 */
void hg_shutdown(
    HurdyGurdy* hg
);

#endif // HG_INIT_H

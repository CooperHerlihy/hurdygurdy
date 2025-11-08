#ifndef HG_INIT_H
#define HG_INIT_H

/**
 * The context for the HurdyGurdy library
 */
typedef struct HurdyGurdy HurdyGurdy;

/**
 * Initializes HurdyGurdy
 *
 * \param hg A pointer to store HurdyGurdy's context, must not be NULL
 * \return HG_SUCCESS if the context was created successfully
 */
HgError* hg_init(
    HurdyGurdy* hg
);

/**
 * Shuts down HurdyGurdy
 *
 * \param hg The graphics context, must not be NULL
 */
void hg_shutdown(
    HurdyGurdy* hg
);

#endif // HG_INIT_H

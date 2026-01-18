#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque pool handle */
typedef struct bank_pool *bank_pool_handle_t;

/**
 * @brief Register a pool with the bank.
 *
 * @param out_handle   Returned handle for this pool
 * @param object_array Pointer to user-owned object array
 * @param object_size  Size of each object
 * @param object_count Number of objects in array
 *
 * @return ESP_OK on success
 *         ESP_ERR_NO_MEM if max pools reached
 *         ESP_ERR_INVALID_ARG on bad input
 */
esp_err_t bank_register_pool(bank_pool_handle_t *out_handle,
                             void *object_array,
                             size_t object_size,
                             size_t object_count);

/**
 * @brief Allocate one object from a pool (best effort).
 *
 * @return Pointer to object or NULL if none available
 */
void *bank_alloc(bank_pool_handle_t handle);

/**
 * @brief Free an object back to its pool.
 *
 * @note Object must belong to the pool.
 */
void bank_free(bank_pool_handle_t handle, void *obj);

#ifdef __cplusplus
}
#endif

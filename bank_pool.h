#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque pool handle */
typedef struct bank_pool *bank_pool_handle_t;

/*
 * Metadata size macro.
 * User must allocate this much memory for each pool.
 */
#define BANK_POOL_META_SIZE(obj_count) \
    (sizeof(struct bank_pool) + (obj_count))

/*
 * Register a pool with the bank.
 *
 * object_array : pointer to array of objects (owned by user)
 * object_size  : sizeof(single object)
 * object_count : number of objects in array
 * meta_buffer  : uint8_t buffer of size BANK_POOL_META_SIZE(object_count)
 *
 * out_handle   : returned pool handle
 */
esp_err_t bank_register_pool(bank_pool_handle_t *out_handle,
                             void *object_array,
                             size_t object_size,
                             size_t object_count,
                             void *meta_buffer);

/*
 * Allocate one object from a pool.
 * Best-effort: returns NULL if none available.
 */
void *bank_alloc(bank_pool_handle_t handle);

/*
 * Free an object back to the pool.
 * Object must belong to this pool.
 */
void bank_free(bank_pool_handle_t handle, void *obj);

#ifdef __cplusplus
}
#endif

#include "bank.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define TAG "BANK"

#define BANK_MAX_POOLS    CONFIG_BANK_MAX_POOLS
#define BANK_MAX_OBJECTS CONFIG_BANK_MAX_OBJECTS_PER_POOL

/* -------- Private pool metadata -------- */

struct bank_pool {
    void *base;
    size_t obj_size;
    size_t obj_count;
    SemaphoreHandle_t mutex;
    uint8_t in_use[BANK_MAX_OBJECTS];
};

/* -------- Global registry -------- */

static struct bank_pool g_pools[BANK_MAX_POOLS];
static size_t g_pool_count;
static SemaphoreHandle_t g_registry_mutex;

/* -------- Init -------- */

__attribute__((constructor))
static void bank_init(void)
{
    g_registry_mutex = xSemaphoreCreateMutex();
    g_pool_count = 0;
    memset(g_pools, 0, sizeof(g_pools));
}

/* -------- API -------- */

esp_err_t bank_register_pool(bank_pool_handle_t *out_handle,
                             void *object_array,
                             size_t object_size,
                             size_t object_count)
{
    if (!out_handle || !object_array ||
        object_size == 0 || object_count == 0 ||
        object_count > BANK_MAX_OBJECTS) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(g_registry_mutex, portMAX_DELAY);

    if (g_pool_count >= BANK_MAX_POOLS) {
        xSemaphoreGive(g_registry_mutex);
        ESP_LOGE(TAG, "Max pool count reached");
        return ESP_ERR_NO_MEM;
    }

    struct bank_pool *pool = &g_pools[g_pool_count++];

    pool->base = object_array;
    pool->obj_size = object_size;
    pool->obj_count = object_count;
    pool->mutex = xSemaphoreCreateMutex();
    memset(pool->in_use, 0, sizeof(pool->in_use));

    *out_handle = pool;

    xSemaphoreGive(g_registry_mutex);

    ESP_LOGI(TAG, "Registered pool %p (%u objects)",
             pool, (unsigned)object_count);

    return ESP_OK;
}

void *bank_alloc(bank_pool_handle_t handle)
{
    if (!handle) {
        return NULL;
    }

    struct bank_pool *pool = handle;

    if (xSemaphoreTake(pool->mutex, 0) != pdTRUE) {
        return NULL;
    }

    for (size_t i = 0; i < pool->obj_count; i++) {
        if (!pool->in_use[i]) {
            pool->in_use[i] = 1;
            xSemaphoreGive(pool->mutex);
            return (uint8_t *)pool->base + (i * pool->obj_size);
        }
    }

    xSemaphoreGive(pool->mutex);
    return NULL;
}

void bank_free(bank_pool_handle_t handle, void *obj)
{
    if (!handle || !obj) {
        return;
    }

    struct bank_pool *pool = handle;

    uintptr_t base = (uintptr_t)pool->base;
    uintptr_t ptr  = (uintptr_t)obj;
    uintptr_t end  = base + pool->obj_size * pool->obj_count;

    if (ptr < base || ptr >= end) {
        ESP_LOGE(TAG, "Invalid free: %p", obj);
        return;
    }

    size_t index = (ptr - base) / pool->obj_size;

    xSemaphoreTake(pool->mutex, portMAX_DELAY);
    pool->in_use[index] = 0;
    xSemaphoreGive(pool->mutex);
}

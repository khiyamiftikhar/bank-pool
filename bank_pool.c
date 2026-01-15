#include "bank.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define TAG "BANK"

/* Pool metadata structure (private) */
struct bank_pool {
    void *base;
    size_t obj_size;
    size_t obj_count;
    SemaphoreHandle_t mutex;
    uint8_t in_use[];
};

/* Registry of pools */
#define BANK_MAX_POOLS CONFIG_BANK_MAX_POOLS

static struct bank_pool *g_pool_registry[BANK_MAX_POOLS];
static size_t g_pool_count;
static SemaphoreHandle_t g_registry_mutex;

/* Called automatically when component is loaded */
__attribute__((constructor))
static void bank_init(void)
{
    g_registry_mutex = xSemaphoreCreateMutex();
    g_pool_count = 0;
}

/* Register a pool */
esp_err_t bank_register_pool(bank_pool_handle_t *out_handle,
                             void *object_array,
                             size_t object_size,
                             size_t object_count,
                             void *meta_buffer)
{
    if (!out_handle || !object_array || !meta_buffer ||
        object_size == 0 || object_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(g_registry_mutex, portMAX_DELAY);

    if (g_pool_count >= BANK_MAX_POOLS) {
        xSemaphoreGive(g_registry_mutex);
        ESP_LOGE(TAG, "Maximum number of pools reached");
        return ESP_ERR_NO_MEM;
    }

    struct bank_pool *pool = (struct bank_pool *)meta_buffer;

    pool->base = object_array;
    pool->obj_size = object_size;
    pool->obj_count = object_count;
    pool->mutex = xSemaphoreCreateMutex();

    memset(pool->in_use, 0, object_count);

    g_pool_registry[g_pool_count++] = pool;
    *out_handle = pool;

    xSemaphoreGive(g_registry_mutex);

    ESP_LOGI(TAG, "Registered pool %p (%u objects)", pool, (unsigned)object_count);
    return ESP_OK;
}

/* Allocate object */
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

/* Free object */
void bank_free(bank_pool_handle_t handle, void *obj)
{
    if (!handle || !obj) {
        return;
    }

    struct bank_pool *pool = handle;

    uintptr_t base = (uintptr_t)pool->base;
    uintptr_t ptr  = (uintptr_t)obj;

    if (ptr < base || ptr >= base + pool->obj_size * pool->obj_count) {
        ESP_LOGE(TAG, "Invalid object pointer %p", obj);
        return;
    }

    size_t index = (ptr - base) / pool->obj_size;

    xSemaphoreTake(pool->mutex, portMAX_DELAY);
    pool->in_use[index] = 0;
    xSemaphoreGive(pool->mutex);
}

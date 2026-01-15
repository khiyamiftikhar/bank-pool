# Bank Resource Manager (ESP-IDF)

## Overview

**Bank** is a small, reusable ESP-IDF component that provides a **centralized,
static, thread-safe object pool allocator**.

It is designed for embedded systems where:
- `malloc/free` are undesirable
- object sizes are fixed
- lifetimes are predictable
- multiple components need pooled resources

The Bank does **not** own object memory.
It only manages **metadata** (in-use tracking + mutex).

---

## Key Features

- Supports **multiple independent pools**
- Maximum number of pools is fixed via **Kconfig**
- **No dynamic memory allocation**
- Thread-safe using FreeRTOS mutexes
- Clean, minimal public API
- Suitable for async systems (HTTP, logging, drivers, etc.)

---

## Design Philosophy

> Data belongs to components.  
> Ownership tracking belongs to the bank.

Each component:
- defines its own object array
- allocates a metadata buffer
- registers the pool once
- uses `bank_alloc()` / `bank_free()`

The bank:
- tracks availability
- enforces limits
- provides mutual exclusion

---

## Public API

```c
esp_err_t bank_register_pool(bank_pool_handle_t *out_handle,
                             void *object_array,
                             size_t object_size,
                             size_t object_count,
                             void *meta_buffer);

void *bank_alloc(bank_pool_handle_t handle);
void bank_free(bank_pool_handle_t handle, void *obj);
```

---

## Metadata Buffer

The user must allocate a metadata buffer of size:

```c
BANK_POOL_META_SIZE(object_count)
```

Example:

```c
static my_type_t objects[8];
static uint8_t meta[BANK_POOL_META_SIZE(8)];
static bank_pool_handle_t pool;
```

---

## Example Usage

```c
typedef struct {
    int id;
    char data[32];
} my_obj_t;

#define OBJ_COUNT 4

static my_obj_t obj_array[OBJ_COUNT];
static uint8_t obj_meta[BANK_POOL_META_SIZE(OBJ_COUNT)];
static bank_pool_handle_t obj_pool;

void init(void)
{
    bank_register_pool(&obj_pool,
                       obj_array,
                       sizeof(my_obj_t),
                       OBJ_COUNT,
                       obj_meta);
}

void example(void)
{
    my_obj_t *obj = bank_alloc(obj_pool);
    if (obj) {
        obj->id = 1;
        bank_free(obj_pool, obj);
    }
}
```

---

## Configuration

Configure maximum number of pools in **menuconfig**:

```
Component config → Bank Resource Manager → Maximum number of pools
```

---

## Notes

- Allocation is **best-effort** (`NULL` if unavailable)
- Objects must only be freed to the pool they came from
- Double-free detection is not enabled by default
- Suitable for use in tasks (not ISRs)

---

## License

This component is intended as a reusable internal utility.
You may adapt or extend it freely for your projects.

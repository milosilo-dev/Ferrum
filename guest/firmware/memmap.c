#include <stdint.h>
#define MEMMAP_MAX_ENTRIES 16

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} MemMapEntry;

MemMapEntry memmap[MEMMAP_MAX_ENTRIES];


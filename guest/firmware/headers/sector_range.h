#pragma once
#include <stdint.h>

typedef struct {
    uint64_t first_sector;
    uint64_t last_sector;
} __attribute__((packed)) SectorRange;
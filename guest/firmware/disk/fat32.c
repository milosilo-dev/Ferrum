#include "../headers/sector_range.h"
#include "../headers/errors.h"

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint32_t fat_size;
    uint32_t root_cluster;
} FAT32_BPB;

typedef struct {
    char name[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) DirEntry;

typedef struct {
    FAT32_BPB* fat;
    SectorRange* range;
    uint32_t data_start_lba;
    uint32_t cluster;
    uint32_t cluster_size;
} __attribute__((packed)) Fat32_Handle;

int open_fat32(SectorRange* range, Fat32_Handle* fs) {
    uint8_t bpb[512];
    int status = virtio_blk_read(range->first_sector + 0, 512, bpb);
    if (status != 0) {
        serial_puts("fat32: could not read BPB\n");
        return IO_ERROR;
    }

    FAT32_BPB fat;
    fat.bytes_per_sector   = *(uint16_t*)(bpb + 11);
    fat.sectors_per_cluster= *(uint8_t*)(bpb + 13);
    fat.reserved_sectors   = *(uint16_t*)(bpb + 14);
    fat.num_fats           = *(uint8_t*)(bpb + 16);
    fat.fat_size           = *(uint32_t*)(bpb + 36);
    fat.root_cluster       = *(uint32_t*)(bpb + 44);

    if (fat.bytes_per_sector != 512) {
        serial_puts("fat32: Invalid sector size of ");
        serial_putx(fat.bytes_per_sector);
        serial_puts("\n");
        return INVALID_FAT_SECTOR_SIZE;
    }

    if (fat.sectors_per_cluster == 0) {
        serial_puts("fat32: Invalid cluster size of 0\n");
        return INVALID_FAT_CLUSTER_SIZE;
    }

    uint32_t data_start_lba = fat.reserved_sectors + fat.num_fats * fat.fat_size;
    uint32_t cluster = fat.root_cluster;
    uint32_t cluster_size = fat.sectors_per_cluster * fat.bytes_per_sector;

    fs->cluster = cluster;
    fs->cluster_size = cluster_size;
    fs->data_start_lba = data_start_lba;
    fs->fat = &fat;
    fs->range = range;

    return SUCCSESS;
}

int open_root_dir(Fat32_Handle* fs) {
    uint8_t cluster_buf[fs->fat->sectors_per_cluster * 512];
    uint32_t lba = fs->data_start_lba +
           (fs->cluster - 2) * fs->fat->sectors_per_cluster;

    uint32_t status = virtio_blk_read(fs->range->first_sector + lba,
        fs->fat->sectors_per_cluster * 512,
        cluster_buf);
    if (status != 0) {
        serial_puts("fat32: Failed to read root directory\n");
        return IO_ERROR;
    }

    serial_puts("fat32: directory contence [");
    for (int i = 0; i < (fs->cluster_size / 32); i++) {
        DirEntry* e = (DirEntry*)(cluster_buf + i * 32);

        if (e->name[0] == 0x00)
            break; // end

        if (e->name[0] == 0xE5)
            continue; // deleted

        if (i != 0) serial_puts(" ,");
        serial_puts(e->name);
    }
    serial_puts("]\n");

    return SUCCSESS;
}
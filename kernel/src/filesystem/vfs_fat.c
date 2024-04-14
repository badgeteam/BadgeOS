
// SPDX-License-Identifier: MIT

#include "filesystem/vfs_fat.h"

// Try to mount a FAT filesystem.
void vfs_fat_mount(badge_err_t *ec, vfs_t *vfs) {
    // Read BPB
    fat_bpb_t bpb;
    uint32_t root_dir_sectors, sectors_per_fat, total_sectors, data_sector_cnt, cluster_count, first_data_sector;
    blkdev_read_partial(ec, vfs->media, 0, 0, (void *)&bpb, sizeof(fat_bpb_t));
    
    // Determine FAT type
    // Root Entry Count is 0 on FAT32
    if(bpb.root_entry_count == 0) {
        fat32_header_t fat32_header;
        root_dir_sectors = 0;
        blkdev_read_partial(ec, vfs->media, 0, 36, (void *)&fat32_header, sizeof(fat32_header_t));
        sectors_per_fat = fat32_header.sectors_per_fat_32;
        total_sectors = bpb.sector_count_32;
        data_sector_cnt = total_sectors - (bpb.reserved_sector_count + (bpb.fat_count * sectors_per_fat));
    }
    else {
        // FAT12/FAT16
        fat16_header_t fat16_header;
        root_dir_sectors = ((bpb.root_entry_count * 32) + (bpb.bytes_per_sector - 1)) / bpb.bytes_per_sector;
        blkdev_read_partial(ec, vfs->media, 0, 36, (void *)&fat16_header, sizeof(fat16_header_t));
        sectors_per_fat = bpb.sectors_per_fat_16;
        total_sectors = bpb.sector_count_16;
        data_sector_cnt = total_sectors - (bpb.reserved_sector_count + (bpb.fat_count * sectors_per_fat) + root_dir_sectors);
    }
    
    // Calculate some parameters
    cluster_count = data_sector_cnt / bpb.sectors_per_cluster;
    first_data_sector = bpb.reserved_sector_count + (bpb.fat_count * sectors_per_fat) + root_dir_sectors;
    
    // Store disk parameters
    vfs->type                    = FS_TYPE_FAT;
    vfs->fat.bytes_per_sector    = bpb.bytes_per_sector;
    vfs->fat.sectors_per_cluster = bpb.sectors_per_cluster;
    vfs->fat.data_sector         = first_data_sector;
    vfs->fat.fat_sector          = bpb.reserved_sector_count;
    vfs->fat.cluster_count       = cluster_count;
}

// Unmount a FAT filesystem.
void vfs_fat_umount(vfs_t *vfs) {

}

// Identify whether a block device contains a FAT filesystem.
// Returns false on error.
bool vfs_fat_detect(badge_err_t *ec, blkdev_t *dev) {
    badge_err_t ec0;
    if (!ec)
        ec = &ec0;

    // Read BPB.
    fat_bpb_t bpb;
    blkdev_read_partial(ec, dev, 0, 0, (void *)&bpb, sizeof(fat_bpb_t));
    if (!badge_err_is_ok(ec))
        return false;

    // Checked signature #1: valid sector size.
    // Sector size is an integer power of 2 from 512 to 4096.
    if (bpb.bytes_per_sector < 512 || bpb.bytes_per_sector > 4096) {
        return false;
    }
    if (bpb.bytes_per_sector & (bpb.bytes_per_sector - 1)) {
        return false;
    }

    // Checked signature #2: FAT BPB signature bytes.
    uint8_t tmp[2];
    blkdev_read_partial(ec, dev, 0, 510, tmp, sizeof(tmp));
    if (!badge_err_is_ok(ec))
        return false;
    if (tmp[0] != 0x55 || tmp[1] != 0xAA) {
        return false;
    }

    // Both minor signature checks passed, this filesystem is probably FAT.
    return true;
}



// Atomically read all directory entries and cache them into the directory handle.
// Refer to `dirent_t` for the structure of the cache.
void vfs_fat_dir_read(badge_err_t *ec, vfs_file_handle_t *dir);
// Open a file for reading and/or writing.
void vfs_fat_file_open(badge_err_t *ec, vfs_file_shared_t *file, char const *path, oflags_t oflags);
// Clone a file opened by `vfs_fat_file_open`.
// Only raises an error if `file` is an invalid file descriptor.
void vfs_fat_file_close(badge_err_t *ec, vfs_file_shared_t *file);
// Read bytes from a file.
void vfs_fat_file_read(badge_err_t *ec, vfs_file_shared_t *file, fileoff_t offset, uint8_t *readbuf, fileoff_t readlen);
// Write bytes from a file.
void vfs_fat_file_write(
    badge_err_t *ec, vfs_file_shared_t *file, fileoff_t offset, uint8_t const *writebuf, fileoff_t writelen
);
// Change the length of a file opened by `vfs_fat_file_open`.
void vfs_fat_file_resize(badge_err_t *ec, vfs_file_shared_t *file, fileoff_t new_size);

// Commit all pending writes to disk.
// The filesystem, if it does caching, must always sync everything to disk at once.
void vfs_fat_flush(badge_err_t *ec, vfs_fat_t *vfs);

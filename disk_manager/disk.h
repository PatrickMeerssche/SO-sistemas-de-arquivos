#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include "fs_types.h"

// Mounts the disk image at the specified path. If the file does not exist or has an incorrect size, 
// it is created and formatted as a new filesystem. If it exists and is valid, it is mounted for use,
// maintaining the existing data. Returns 0 on success, -1 on failure. 
int disk_mount(const char *path);

// Unmounts the disk image by synchronizing any pending changes, unmapping the memory, and closing the file descriptor.
void disk_unmount(void);

// Synchronizes the disk image by persisting any pending changes to the underlying file.
void disk_sync(void);

// Pointers to the structures within the memory-mapped disk image.
uint8_t *disk_block_ptr(uint32_t block_num);
superblock_t *disk_superblock(void);
uint8_t *disk_block_bitmap(void);
uint8_t *disk_inode_bitmap(void);
inode_t *disk_inode_table(void);

#endif

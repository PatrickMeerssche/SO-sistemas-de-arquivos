#include "superblock.h"
#include "bitmap.h"
#include "disk.h"
#include "inode.h"
#include "fs_types.h"

#include <string.h>
#include <time.h>

// Helper function to perform ceiling division of two unsigned integers.
static uint32_t ceil_div(uint32_t a, uint32_t b) {
    return (a + b - 1) / b;
}

// Format the disk with a new filesystem, initializing the superblock, bitmaps, and inode table.
void superblock_format(void) {
    // Get a pointer to the superblock structure in memory and clear it.
    superblock_t *sb = disk_superblock();
    memset(sb, 0, sizeof(superblock_t));

    // Set the basic filesystem metadata in the superblock.  
    sb->magic = SUPERBLOCK_MAGIC;
    sb->block_size = BLOCK_SIZE;
    sb->total_blocks = TOTAL_BLOCKS;
    sb->num_inodes = NUM_INODES;

    // Block layout information
    sb->block_bitmap_start = 1;     // block 0 is reserved for the superblock itself
    sb->block_bitmap_blocks = ceil_div(ceil_div(TOTAL_BLOCKS, 8), BLOCK_SIZE);

    // Inode bitmap starts immediately after the block bitmap, and its size is calculated based on the number of inodes.
    sb->inode_bitmap_start = sb->block_bitmap_start + sb->block_bitmap_blocks;
    sb->inode_bitmap_blocks = ceil_div(ceil_div(NUM_INODES, 8), BLOCK_SIZE);

    // Inode table starts immediately after the inode bitmap, and its size is calculated based on the number of inodes and the size of each inode structure.
    sb->inode_table_start = sb->inode_bitmap_start + sb->inode_bitmap_blocks;
    sb->inode_table_blocks = ceil_div(NUM_INODES * (uint32_t)sizeof(inode_t), BLOCK_SIZE);

    // The data area starts immediately after the inode table.  
    sb->data_start = sb->inode_table_start + sb->inode_table_blocks;

    // Initialize the bitmaps and inode table to zero.
    memset(disk_block_bitmap(), 0, (size_t)sb->block_bitmap_blocks * BLOCK_SIZE);
    memset(disk_inode_bitmap(), 0, (size_t)sb->inode_bitmap_blocks * BLOCK_SIZE);
    memset(disk_inode_table(), 0, (size_t)sb->inode_table_blocks * BLOCK_SIZE);

    // Mark all blocks before the data area as used in the block bitmap, since they are reserved for filesystem metadata (superblock, bitmaps, inode table).
    uint8_t *bmap = disk_block_bitmap();
    for (uint32_t i = 0; i < sb->data_start; i++) {
        bitmap_set(bmap, i);
    }

    // Initialize the counts of free blocks and inodes in the superblock.
    sb->free_inodes = sb->num_inodes;
    sb->root_inode = 0;

    // Allocate the root inode, initialize its fields, and set it as the root directory of the filesystem.
    int32_t root = inode_alloc();
    inode_t *root_inode = inode_get((uint32_t)root);
    root_inode->type = INODE_DIR;
    strncpy(root_inode->name, "/", MAX_NAME_LEN - 1);
    strncpy(root_inode->creator, "root", MAX_USER_LEN - 1);
    strncpy(root_inode->owner, "root", MAX_USER_LEN - 1);
    
    // Set the permissions for the root inode to allow read, write, and execute for both the owner and others.
    root_inode->perm_owner = PERM_READ | PERM_WRITE | PERM_EXEC;
    root_inode->perm_other = PERM_READ | PERM_WRITE | PERM_EXEC;
    root_inode->created_at = time(NULL);
    root_inode->modified_at = root_inode->created_at;
    
    root_inode->parent_inode = 0;       // The root directory is its own parent.
    sb->root_inode = (uint32_t)root;    // Set the root inode number in the superblock.
}

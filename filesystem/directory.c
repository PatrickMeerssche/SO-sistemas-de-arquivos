#include "directory.h"
#include "../disk_manager/disk.h"
#include "../disk_manager/inode.h"
#include "perm.h"

#include <string.h>
#include <time.h>

// Find a directory entry named `name` in the directory represented by `dir_idx`.
int dir_find_entry(uint32_t dir_idx, const char *name, uint32_t *out_child) {
    inode_t *dir = inode_get(dir_idx);
    uint32_t num_blocks = dir->size / BLOCK_SIZE;

    for (uint32_t b = 0; b < num_blocks; b++) {
        uint32_t blk = inode_get_block(dir, b, 0);
        dirent_t *entries = (dirent_t *)disk_block_ptr(blk);
        for (uint32_t e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (!entries[e].used) {
                continue;
            }
            inode_t *child = inode_get((uint32_t)entries[e].inode_num);
            if (strncmp(child->name, name, MAX_NAME_LEN) == 0) {
                *out_child = (uint32_t)entries[e].inode_num;
                return 1;
            }
        }
    }
    return 0;
}

// Add a directory entry pointing to `child_idx` in the directory represented by `dir_idx`.
int dir_add_entry(uint32_t dir_idx, uint32_t child_idx) {
    inode_t *dir = inode_get(dir_idx);
    uint32_t num_blocks = dir->size / BLOCK_SIZE;

    // First, try to find a free entry in the existing blocks.
    for (uint32_t b = 0; b < num_blocks; b++) {
        uint32_t blk = inode_get_block(dir, b, 0);
        dirent_t *entries = (dirent_t *)disk_block_ptr(blk);
        for (uint32_t e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (!entries[e].used) {
                entries[e].used = 1;
                entries[e].inode_num = (int32_t)child_idx;
                return 0;
            }
        }
    }

    // If no free entry was found, we need to allocate a new block for the directory.
    uint32_t blk = inode_get_block(dir, num_blocks, 1);
    if (blk == INVALID_BLOCK) {
        return -1; // Disk is full or cannot allocate a new block.
    }
    dir->size += BLOCK_SIZE;

    dirent_t *entries = (dirent_t *)disk_block_ptr(blk);
    entries[0].used = 1;
    entries[0].inode_num = (int32_t)child_idx;
    return 0;
}

// Remove the directory entry that points to `child_idx` from the directory represented by `dir_idx`.
int dir_remove_entry(uint32_t dir_idx, uint32_t child_idx) {
    inode_t *dir = inode_get(dir_idx);
    uint32_t num_blocks = dir->size / BLOCK_SIZE;

    for (uint32_t b = 0; b < num_blocks; b++) {
        uint32_t blk = inode_get_block(dir, b, 0);
        dirent_t *entries = (dirent_t *)disk_block_ptr(blk);
        for (uint32_t e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (entries[e].used && (uint32_t)entries[e].inode_num == child_idx) {
                entries[e].used = 0;
                entries[e].inode_num = -1;
                return 0;
            }
        }
    }
    return -1; // Entry not found
}

// Return 1 if the directory has no children, 0 otherwise.
int dir_is_empty(uint32_t dir_idx) {
    inode_t *dir = inode_get(dir_idx);
    uint32_t num_blocks = dir->size / BLOCK_SIZE;

    for (uint32_t b = 0; b < num_blocks; b++) {
        uint32_t blk = inode_get_block(dir, b, 0);
        dirent_t *entries = (dirent_t *)disk_block_ptr(blk);
        for (uint32_t e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (entries[e].used) {
                return 0;
            }
        }
    }
    return 1;
}

// Call cb(child_inode_idx, ctx) for each child entry in the directory represented by `dir_idx`.
void dir_for_each(uint32_t dir_idx, dir_iter_cb cb, void *ctx) {
    inode_t *dir = inode_get(dir_idx);
    uint32_t num_blocks = dir->size / BLOCK_SIZE;

    for (uint32_t b = 0; b < num_blocks; b++) {
        uint32_t blk = inode_get_block(dir, b, 0);
        dirent_t *entries = (dirent_t *)disk_block_ptr(blk);
        for (uint32_t e = 0; e < ENTRIES_PER_BLOCK; e++) {
            if (entries[e].used) {
                cb((uint32_t)entries[e].inode_num, ctx);
            }
        }
    }
}

// Allocate a new inode of type `type` named `name` within `parent_idx` and add it as a directory entry. Fill in owner/creator/permissions/timestamps. Return 0 on success. Negative error codes: -1 parent is not a directory, -2 permission denied, -3 name already exists, -4 no free inodes, -5 disk full when growing the parent directory.
int dir_create_child(uint32_t parent_idx, const char *name, uint8_t type,
                      const char *user, uint32_t *out_idx) {
    inode_t *parent = inode_get(parent_idx);
    if (parent->type != INODE_DIR) {
        return -1;
    }
    if (!has_permission(parent, user, PERM_WRITE)) {
        return -2;
    }

    uint32_t existing;
    if (dir_find_entry(parent_idx, name, &existing)) {
        return -3;
    }

    int32_t idx = inode_alloc();
    if (idx < 0) {
        return -4;
    }

    inode_t *inode = inode_get((uint32_t)idx);
    strncpy(inode->name, name, MAX_NAME_LEN - 1);
    strncpy(inode->creator, user, MAX_USER_LEN - 1);
    strncpy(inode->owner, user, MAX_USER_LEN - 1);
    inode->type = type;
    inode->created_at = time(NULL);
    inode->modified_at = inode->created_at;

    // Every parent directory should have a reference to its child, and every child should have a reference to its parent. 
    inode->parent_inode = (int32_t)parent_idx;

    if (type == INODE_DIR) {
        inode->perm_owner = PERM_READ | PERM_WRITE | PERM_EXEC;
        inode->perm_other = PERM_READ | PERM_EXEC;
    } else {
        // For files and other types, set default permissions (read/write for owner, read for others).
        inode->perm_owner = PERM_READ | PERM_WRITE;
        inode->perm_other = PERM_READ;
    }

    if (dir_add_entry(parent_idx, (uint32_t)idx) != 0) {
        inode_free((uint32_t)idx);
        return -5;
    }

    *out_idx = (uint32_t)idx;
    return 0;
}

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "../disk_manager/fs_types.h"

// Find a directory entry named `name` in the directory represented by `dir_idx`.
int dir_find_entry(uint32_t dir_idx, const char *name, uint32_t *out_child);

// Add a directory entry pointing to `child_idx` in the directory represented by `dir_idx`.
int dir_add_entry(uint32_t dir_idx, uint32_t child_idx);

// Remove the directory entry that points to `child_idx` from the directory represented by `dir_idx`.
int dir_remove_entry(uint32_t dir_idx, uint32_t child_idx);

// Return 1 if the directory has no children, 0 otherwise.
int dir_is_empty(uint32_t dir_idx);

// Call cb(child_inode_idx, ctx) para cada entrada de filho no diretorio representado por `dir_idx`.
typedef void (*dir_iter_cb)(uint32_t child_idx, void *ctx);

// Call cb(child_inode_idx, ctx) for each child entry in the directory represented by `dir_idx`.
void dir_for_each(uint32_t dir_idx, dir_iter_cb cb, void *ctx);

// Allocate a new inode of type `type` named `name` within `parent_idx` and add it as a directory entry. Fill in owner/creator/permissions/timestamps. Return 0 on success. Negative error codes: -1 parent is not a directory, -2 permission denied, -3 name already exists, -4 no free inodes, -5 disk full when growing the parent directory.
int dir_create_child(uint32_t parent_idx, const char *name, uint8_t type,
                      const char *user, uint32_t *out_idx);

#endif

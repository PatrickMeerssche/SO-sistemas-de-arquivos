#ifndef DIR_OPS_H
#define DIR_OPS_H

#include "../disk_manager/fs_types.h"

// Create a new directory at `path` relative to `cwd`. The new directory will be owned by `user`.
int fs_mkdir(uint32_t cwd, const char *path, const char *user);

// Remove the directory at `path` relative to `cwd`. The directory must be empty.
int fs_rmdir(uint32_t cwd, const char *path, const char *user);

// List the contents of the directory at `path` relative to `cwd`. If `path` is empty, list the contents of `cwd`.
int fs_ls(uint32_t cwd, const char *path, const char *user);

// Solve `path` relative to `cwd` and change the current working directory to that path. 
// The new working directory will be returned in `out_inode`.
int fs_cd(uint32_t cwd, const char *path, const char *user, uint32_t *out_inode);

#endif

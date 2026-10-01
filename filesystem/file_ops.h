#ifndef FILE_OPS_H
#define FILE_OPS_H

#include "../disk_manager/fs_types.h"

// Create a new file or update the modification time of an existing file.
int fs_touch(uint32_t cwd, const char *path, const char *user);

// Remove a file.
int fs_rm(uint32_t cwd, const char *path, const char *user);

// Write `content` to the file at `path`. If `append` is non-zero, append to the file; otherwise, overwrite it.
int fs_write_content(uint32_t cwd, const char *path, const char *content, int append, const char *user);

// Display the content of the file at `path` to standard output.
int fs_cat(uint32_t cwd, const char *path, const char *user);

// Copy the file from `src` to `dst`, creating `dst` if necessary. If `dst` is an existing directory, the source file is copied into that directory with the same name.
int fs_cp(uint32_t cwd, const char *src, const char *dst, const char *user);

// Move the file or directory from `src` to `dst`. If `dst` is an existing directory, the source is moved into that directory with the same name.
int fs_mv(uint32_t cwd, const char *src, const char *dst, const char *user);

// Create a symbolic link named `linkname` pointing to `target`.
int fs_ln(uint32_t cwd, const char *target, const char *linkname, const char *user);

#endif

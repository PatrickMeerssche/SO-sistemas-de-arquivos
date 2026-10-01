#ifndef PATH_H
#define PATH_H

#include "../disk_manager/fs_types.h"
#include <stddef.h>

#define MAX_PATH_LEN 512
#define MAX_DEPTH 64 // Maximum number of components in a path

// Split a path into its components, storing them in `comps`. Returns the number of components found.
// For example, "/a/b/c" would yield ["a", "b", "c"] with a return value of 3.
int path_split(const char *path, char comps[MAX_DEPTH][MAX_NAME_LEN]);

// Follow a chain of symlinks starting from `link_idx`, resolving relative targets
// with respect to the link's parent directory. Returns the final inode that is not a link
// or INVALID_INODE in case of a broken link or too many jumps (protection against loops).
int path_resolve(const char *path, uint32_t base_inode, int follow_final_link,
                  uint32_t *out_inode, uint32_t *out_parent, char *out_name);

// Build the absolute path of `inode_idx` (e.g., "/a/b/c") into `out`.
// The function traverses up the directory tree to construct the path, stopping at the root inode.
void path_absolute(uint32_t inode_idx, char *out, size_t out_sz);

#endif

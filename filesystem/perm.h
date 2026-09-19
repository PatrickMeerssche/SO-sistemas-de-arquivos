#ifndef PERM_H
#define PERM_H

#include "../disk_manager/fs_types.h"

// Verify if `user` has all the permission bits specified in `need` (PERM_READ, PERM_WRITE, PERM_EXEC) for the given `inode`.
// If `user` is the owner of the inode, the function checks against `perm_owner`;
// There is no login system yet, so "user" is simply the name with which the shell was started.
int has_permission(const inode_t *inode, const char *user, int need);

#endif

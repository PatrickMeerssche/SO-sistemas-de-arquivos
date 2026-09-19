#include "perm.h"

#include <string.h>

// Verify if `user` has all the permission bits specified in `need` (PERM_READ, PERM_WRITE, PERM_EXEC) for the given `inode`.
// If `user` is the owner of the inode, the function checks against `perm_owner`;
// There is no login system yet, so "user" is simply the name with which the shell was started.
int has_permission(const inode_t *inode, const char *user, int need) {
    int is_owner = (strncmp(inode->owner, user, MAX_USER_LEN) == 0);
    uint8_t perm = is_owner ? inode->perm_owner : inode->perm_other;
    return (perm & need) == need;
}

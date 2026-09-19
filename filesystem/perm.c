#include "perm.h"

#include <string.h>

int has_permission(const inode_t *inode, const char *user, int need) {
    int is_owner = (strncmp(inode->owner, user, MAX_USER_LEN) == 0);
    uint8_t perm = is_owner ? inode->perm_owner : inode->perm_other;
    return (perm & need) == need;
}

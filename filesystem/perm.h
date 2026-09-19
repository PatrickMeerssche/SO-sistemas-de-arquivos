#ifndef PERM_H
#define PERM_H

#include "../disk_manager/fs_types.h"

// Verifica se `user` possui todos os bits de `need` (PERM_READ/WRITE/EXEC)
// sobre `inode`: usa perm_owner se `user` for o dono do i-node, ou
// perm_other caso contrario. Nao existe um sistema de login de verdade
// neste projeto, entao "user" e apenas o nome com que a shell foi iniciada.
int has_permission(const inode_t *inode, const char *user, int need);

#endif

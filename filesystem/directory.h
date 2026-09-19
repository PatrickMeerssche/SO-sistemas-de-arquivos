#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "../disk_manager/fs_types.h"

// Procura `name` entre os filhos diretos do diretorio `dir_idx`.
// Retorna 1 e preenche *out_child se encontrar, 0 caso contrario.
int dir_find_entry(uint32_t dir_idx, const char *name, uint32_t *out_child);

// Adiciona uma entrada de diretorio apontando para `child_idx` dentro de
// `dir_idx`, crescendo o conteudo do diretorio com um novo bloco se nao
// houver posicao livre. Retorna 0 em caso de sucesso, -1 se o disco estiver cheio.
int dir_add_entry(uint32_t dir_idx, uint32_t child_idx);

// Remove a entrada que aponta para `child_idx` de `dir_idx`.
// Retorna 0 em caso de sucesso, -1 se nao for encontrada.
int dir_remove_entry(uint32_t dir_idx, uint32_t child_idx);

// Retorna 1 se o diretorio nao tem filhos, 0 caso contrario.
int dir_is_empty(uint32_t dir_idx);

// Chama cb(child_inode_idx, ctx) para cada entrada filha do diretorio.
typedef void (*dir_iter_cb)(uint32_t child_idx, void *ctx);
void dir_for_each(uint32_t dir_idx, dir_iter_cb cb, void *ctx);

// Aloca um novo i-node do tipo `type` chamado `name` dentro de `parent_idx`
// e o adiciona como entrada de diretorio. Preenche dono/criador/permissoes/
// datas. Retorna 0 em caso de sucesso. Codigos de erro negativos: -1 pai
// nao e um diretorio, -2 permissao negada, -3 nome ja existe, -4 sem
// i-nodes livres, -5 disco cheio ao crescer o diretorio pai.
int dir_create_child(uint32_t parent_idx, const char *name, uint8_t type,
                      const char *user, uint32_t *out_idx);

#endif

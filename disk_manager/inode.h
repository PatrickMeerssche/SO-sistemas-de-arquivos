#ifndef INODE_H
#define INODE_H

// Alocacao de i-nodes e blocos de dados. Tanto arquivos quanto diretorios
// sao apenas "i-nodes com conteudo", entao a mesma logica de encadeamento
// de blocos e compartilhada por ambos.

#include "fs_types.h"

// Aloca um i-node livre, zera seu conteudo e o marca como usado. Retorna seu
// indice, ou -1 se nao houver mais i-nodes livres.
int32_t inode_alloc(void);

// Libera todos os blocos de dados pertencentes ao i-node e depois marca o
// proprio i-node como livre novamente.
void inode_free(uint32_t idx);

// Ponteiro para a posicao do i-node `idx` na tabela de i-nodes em disco.
inode_t *inode_get(uint32_t idx);

// Aloca um bloco de dados livre, zera seu conteudo e o marca como usado.
// Retorna o numero do bloco, ou INVALID_BLOCK se o disco estiver cheio.
uint32_t block_alloc(void);
void block_free(uint32_t block_num);

// Retorna o numero do bloco que guarda o `logical_index`-esimo bloco do
// conteudo de um i-node (a partir de 0). Blocos alem de DIRECT_BLOCKS sao
// alcancados seguindo a cadeia de i-nodes de continuacao apontada por
// `next_inode`. Se `allocate` for diferente de zero, blocos/i-nodes de
// continuacao faltantes sao criados conforme necessario; caso contrario,
// blocos faltantes retornam INVALID_BLOCK.
uint32_t inode_get_block(inode_t *inode, uint32_t logical_index, int allocate);

// Libera todos os blocos de dados (e eventuais i-nodes de continuacao) de
// `inode`, deixando seu conteudo vazio (size volta a 0).
void inode_truncate(inode_t *inode);

#endif

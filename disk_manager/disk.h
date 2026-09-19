#ifndef DISK_H
#define DISK_H

// Camada mais baixa do sistema de arquivos: trata a imagem de disco como um
// vetor de blocos de tamanho fixo, mapeada em memoria (mmap) a partir de um
// arquivo comum, para que as alteracoes possam ser persistidas em disco.

#include "fs_types.h"

// Abre (ou cria) a imagem de disco em `path`. Se o arquivo ainda nao existe
// ou tem o tamanho errado, ele e (re)criado com exatamente DISK_SIZE bytes
// zerados de verdade e formatado com um sistema de arquivos novo. Retorna 0
// em caso de sucesso, -1 em caso de falha.
int disk_mount(const char *path);

// Persiste as alteracoes pendentes em disco (msync) e desfaz o mapeamento.
void disk_unmount(void);

// Persiste as alteracoes pendentes em disco sem desmontar.
void disk_sync(void);

// Ponteiro para os bytes crus do bloco `block_num` dentro da imagem mapeada.
uint8_t *disk_block_ptr(uint32_t block_num);

// Acessos convenientes para as regioes conhecidas da imagem de disco.
superblock_t *disk_superblock(void);
uint8_t *disk_block_bitmap(void);
uint8_t *disk_inode_bitmap(void);
inode_t *disk_inode_table(void);

#endif

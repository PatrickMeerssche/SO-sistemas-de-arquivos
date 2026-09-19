#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

// Calcula o layout em disco, zera os bitmaps/tabela de i-nodes e cria o
// diretorio raiz. Chamada apenas uma vez, quando a imagem e nova.
void superblock_format(void);

#endif

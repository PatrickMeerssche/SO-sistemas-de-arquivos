# SO-sistemas-de-arquivos

Este projeto implementa um sistema de arquivos em C, com armazenamento em uma imagem de disco real em arquivo, estrutura baseada em i-nodes e suporte a diretórios, arquivos, links simbólicos e permissões básicas.

A ideia principal é simular o comportamento de um sistema de arquivos em nível de abstração de disco, mantendo os dados persistidos em um arquivo local e permitindo operações comuns de manipulação de arquivos e diretórios pelo terminal.

## Visão geral

O projeto organiza a lógica em camadas bem separadas:

- `disk_manager/`: responsável pela imagem do disco, bitmap, superbloco, alocação de i-nodes e blocos de dados.
- `filesystem/`: responsável por diretórios, arquivos, caminhos, permissões e operações de leitura/escrita.
- `shell/`: oferece um interpretador de comandos interativo para o usuário.
- `disk/`: diretório onde a imagem do disco é armazenada.

O disco inicializa a primeira vez com zeros, ou seja, começa vazio em termos de estrutura de arquivos, e a imagem tem tamanho de 128 MB.

## Estrutura do projeto

- `main.c`: ponto de entrada do programa.
- `Makefile`: compila o projeto em C.
- `disk_manager/`: gerenciamento e layout do disco.
- `filesystem/`: lógica do sistema de arquivos.
- `shell/`: shell de interação com o usuário.
- `disk/disk_storage`: arquivo de imagem do disco usado pela aplicação.

## Como compilar

Na raiz do projeto, execute:

```bash
make
```

Isso gera o binário `fs`.

## Como executar

```bash
./fs
```

Também é possível passar um caminho customizado para a imagem do disco:

```bash
./fs /caminho/para/arquivo.bin
```

Se o arquivo não existir, o sistema cria uma imagem nova com o tamanho correto. Se ele existir e for válido, o sistema monta a estrutura já persistida.

## Características implementadas

- disco em arquivo com tamanho fixo de 128 MB;
- superbloco com metadados do sistema de arquivos;
- bitmap para blocos e i-nodes;
- tabela de i-nodes;
- diretórios e arquivos com nomes e permissões;
- suporte a criação, remoção, cópia e movimentação de arquivos;
- suporte a criação e remoção de diretórios;
- suporte a links simbólicos (`ln -s`);
- navegação por caminhos relativos e absolutos;
- persistência de alterações após cada comando.

## Comandos disponíveis

O shell aceita os seguintes comandos:

```bash
help
pwd
ls [diretorio]
cd <diretorio>
touch <arquivo>
rm <arquivo>
mkdir <diretorio>
rmdir <diretorio>
cat <arquivo>
echo "texto" > <arquivo>
echo "texto" >> <arquivo>
cp <origem> <destino>
mv <origem> <destino>
ln -s <alvo> <link>
exit
quit
```

## Exemplo de uso

```bash
./fs
Mini sistema de arquivos baseado em i-nodes. Digite 'help' para ajuda.
usuario:/$ mkdir pasta
usuario:/$ cd pasta
usuario:/pasta$ touch arquivo.txt
usuario:/pasta$ echo "hello world" > arquivo.txt
usuario:/pasta$ cat arquivo.txt
hello world
usuario:/pasta$ ls
arquivo.txt
```

## Observações importantes

- O disco é inicialmente preenchido com zeros.
- A aplicação trata a imagem como um container real do sistema de arquivos.
- As alterações são persistidas após cada comando executado no shell.
- A estrutura do projeto foi pensada para ser legível e didática, com foco em aprendizado de sistemas de arquivos em C.

## Arquitetura de dados

A base do sistema segue a ideia de um FS em disco com:

- `superbloco`: guarda informações gerais do sistema e posições de estruturas importantes;
- `bitmap de blocos`: indica quais blocos estão livres ou ocupados;
- `bitmap de i-nodes`: indica quais i-nodes estão livres ou alocados;
- `tabela de i-nodes`: guarda atributos dos arquivos e diretórios;
- `blocos de dados`: armazenam o conteúdo dos arquivos e as entradas de diretórios.

Esse modelo permite a implementação de operações comuns de um sistema de arquivos em um ambiente controlado e fácil de testar.

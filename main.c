#include "disk_manager/disk.h"
#include "shell/shell.h"

#include <stdio.h>

int main(int argc, char **argv) {
    // a imagem de disco por padrao e disk/disk_storage, mas um caminho
    // customizado pode ser passado na linha de comando (util para testes).
    const char *disk_path = (argc > 1) ? argv[1] : "disk/disk_storage";

    if (disk_mount(disk_path) != 0) {
        fprintf(stderr, "erro: nao foi possivel montar o disco em '%s'\n", disk_path);
        return 1;
    }

    shell_run();

    disk_unmount();
    return 0;
}

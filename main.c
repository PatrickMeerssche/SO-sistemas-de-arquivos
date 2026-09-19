#include "disk_manager/disk.h"
#include "shell/shell.h"

#include <stdio.h>

int main(int argc, char **argv) {
    // The default disk image path is "disk/disk_storage", but a custom 
    // path can be provided as a command-line argument.  
    const char *disk_path = (argc > 1) ? argv[1] : "disk/disk_storage";

    // Attempt to mount the disk image. If mounting fails, print an error message and exit with a non-zero status code
    if (disk_mount(disk_path) != 0) {
        fprintf(stderr, "error: could not mount disk at '%s'\n", disk_path);
        return 1;
    }

    // Start the interactive shell loop.
    shell_run();

    // Unmount the disk image before exiting the program.
    disk_unmount();
    return 0;
}

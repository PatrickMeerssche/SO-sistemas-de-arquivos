CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -I.
TARGET = filesystem

SRCS = main.c \
       disk_manager/bitmap.c \
       disk_manager/disk.c \
       disk_manager/superblock.c \
       disk_manager/inode.c \
       filesystem/perm.c \
       filesystem/directory.c \
       filesystem/path.c \
       filesystem/file_ops.c \
       filesystem/dir_ops.c \
       shell/shell.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

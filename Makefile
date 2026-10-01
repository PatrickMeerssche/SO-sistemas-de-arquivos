CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -I.
TARGET = fs

SRCS = main.c \
       disk_manager/bitmap.c \
       disk_manager/disk.c \
       disk_manager/inode.c \
       disk_manager/superblock.c \
       filesystem/dir_ops.c \
       filesystem/directory.c \
       filesystem/file_ops.c \
       filesystem/path.c \
       filesystem/perm.c \
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

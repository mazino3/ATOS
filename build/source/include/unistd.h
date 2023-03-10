#pragma once

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#ifndef COMPILE_KERNEL

#include <sys/types.h>

int close(int fd);

ssize_t read(int fd, void* buffer, size_t size);
ssize_t write(int fd, const void* buffer, size_t size);

off_t lseek(int fd, off_t offset, int whence);

#endif
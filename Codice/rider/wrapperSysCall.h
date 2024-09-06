#ifndef WRAPPERSYSCALL_H
#define WRAPPERSYSCALL

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>

pid_t Fork();
int Open(const char *pathname, int flags, mode_t mode);
size_t Read(int fd, void* buf, size_t count);
ssize_t FullRead(int fd, void *buf, size_t count);
size_t Write (int fd, void* buf, size_t count);
ssize_t FullWrite(int fd, const void *buf, size_t count);
/* Legge dal file ed inserisce il contenuto in un buffer */
char* leggi_da_file(int file);

#endif

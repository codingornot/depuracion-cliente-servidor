#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <debug.h>

void ensuciar(void *buf, size_t tam)
{
  int fd = open("/dev/urandom", O_RDONLY);
  read(fd, buf, tam);
  close(fd);
}


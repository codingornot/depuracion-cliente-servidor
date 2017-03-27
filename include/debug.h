#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#define _breakpoint(file, line) \
  do { int pid = getpid(); \
    printf("El proceso %d alcanzó un breakpoint en %s:%d\n", \
           pid, (file), (line));\
    raise(SIGINT);\
  } while(0)

#define breakpoint _breakpoint(__FILE__, __LINE__)

#endif/*__DEBUG_H__*/

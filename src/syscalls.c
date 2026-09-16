/// 用来消除链接警告
/// _write：printf / puts / fwrite → 输出用
/// _read：scanf / fread → 输入用
/// _close/_fstat/_lseek：文件操作，裸机用不到
/// _kill/_getpid：进程控制，裸机用不到
/// _exit：程序退出，裸机用不到

#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

// 弱定义：默认实现，用户可以覆盖
__attribute__((weak)) int _write(int file, char *ptr, int len)
{
  (void)file;
  (void)ptr;
  (void)len;
  return len;
}

__attribute__((weak)) int _read(int file, char *ptr, int len)
{
  (void)file;
  (void)ptr;
  (void)len;
  return -1;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    (void)st;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    (void)status;
    while (1);
}

int _open(const char *path, int flags, int mode)
{
    (void)path;
    (void)flags;
    (void)mode;
    return -1;
}
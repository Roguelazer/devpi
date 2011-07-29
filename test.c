#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    char buf[20];
    int fd;
    fd = open("/dev/pi", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    read(fd, buf, 10);
    write(STDOUT_FILENO, buf, 10);
    close(fd);
    return 0;
}

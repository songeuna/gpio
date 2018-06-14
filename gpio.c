#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define BUF_SIZE 100

int main(int argc, char **argv)
{
    char buf[BUF_SIZE];
    int fd = -1;
    int count;

    memset(buf, 0, BUF_SIZE);

    printf("GPIO Set : %s\n", argv[1]);

    fd = open("/dev/gpioled", O_RDWR);
    if(fd < 0)
    {
        printf("Error : open failed %d \n", errno);
        system("sudo mknod /dev/gpioled c 200 0");
        system("sudo chmod 666 /dev/gpioled");
        fd = open("/dev/gpioled", O_RDWR);
        if( fd < 0)
            return -1;
    }
    printf("/dev/gpioled opened()\n");

    count = write(fd, argv[1], strlen(argv[1]));
    if(count < 0)
    {
        printf("Error : write()\n");
        return -1;
    }

    count = read(fd, buf, 20);
    printf("Read data : %s\n", buf);

    close(fd);

    printf("/dev/gpioled closed()\n");

    return 0;
}

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#define BUF_SIZE 100
int fd = -1;
char buf[BUF_SIZE];

void signal_handler(int signum)
{
    int count;

    printf("user app : signal is catched\n");
    
    count = read(fd, buf, 20);

    if(signum == SIGIO)
    {
        printf("%s\n",buf);
        //exit(1);
    }
}

int main(int argc, char **argv)
{
    //char buf[BUF_SIZE];
    //int fd = -1;
    int count;

    memset(buf, 0, BUF_SIZE);

    signal(SIGIO, signal_handler);

    printf("GPIO Set : %s\n", argv[1]);

    fd = open("/dev/gpioled", O_RDWR);
    
    if(fd < 0)
    {
        printf("Error : open()\n");
        return -1;
    }
    
   
    /*if(fd < 0)
    {
        printf("Error : open failed %d \n", errno);
        system("sudo mknod /dev/gpioswitch c 200 0");
        system("sudo chmod 666 /dev/gpioswitch");
        fd = open("/dev/gpioswitch", O_RDWR);
        if( fd < 0)
            return -1;
    }*/
    //printf("/dev/gpioled opened()\n");

    sprintf(buf, "%s:%d",argv[1], getpid());
    count = write(fd, buf, strlen(buf));

    //printf("Write data : %s\n", argv[1]);

    if(count < 0)
    {
        printf("Error : write()\n");
       // return -1;
    }

         //printf("Read data : %s\n", buf);

    while(1);
    close(fd);

    //printf("/dev/gpioswitch closed()\n");

    return 0;
}

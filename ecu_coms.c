#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include "ecu.h"

static int fd = -1;

void coms_listen(const char *dev)
{
    char buf[80];
    fd = open(dev, O_RDWR);
    int res = 1;

    if (fd > 0)
    {
        printf("connected...\n");
    }

    while (fd > 0 && res >= 0)
    {
        struct pollfd coms_poll = { fd, POLLIN, 0};
        res = poll(&coms_poll, 1, 1000);
        DBG("res: %i\n", res);

        if (!res)
        {
            write(fd, "keijo\n", 6);
        }

        if (res)
        {
            DBG("revents: %x\n", coms_poll.revents);

            if (coms_poll.revents & POLLIN)
            {
                res = read(fd, buf, sizeof(buf));
                DBG("read: %i\n", res);
                if (res > 0)
                {
                    buf[res] = 0;
                    printf("%s", buf);
                }
            }

            if (coms_poll.revents & (POLLERR | POLLHUP))
            {
                printf("disconnected...\n");
                close(fd);

                sleep(1);
                fd = open(dev, O_RDWR);
                sleep(1);
                close(fd);

                struct stat tmp;
                while (stat(dev, &tmp) == 0)
                {
                    sleep(1);
                }

                fd = -1;
            }
        }
    }

    DBG("exit listener...\n");
}

void coms_write(const char *dev, const char *data, int n)
{
    if (fd < 0)
    {
        fd = open(dev, O_RDWR);
    }

    if (fd > 0)
    {
        int res = write(fd, data, n);
        if (res != n)
        {
            close(fd);

            sleep(1);
            fd = open(dev, O_RDWR);
            sleep(1);
            close(fd);

            struct stat tmp;
            while (stat(dev, &tmp) == 0)
            {
                sleep(1);
            }

            fd = -1;
        } 
    }
}


#ifdef __HAS_MAIN__

int main(void)
{
    while (1)
    {
        //coms_listen("/dev/rfcomm0");
        coms_write("/dev/rfcomm0", "moimoi\n", 7);
        sleep(1);
    }

    return 0;
}

#endif


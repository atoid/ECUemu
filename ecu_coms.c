#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include <pthread.h>
#include <signal.h>

#include "ecu.h"

static const char *dev = "/dev/rfcomm0";
static int fd = -1;
static pthread_t rfcomm;

void coms_listen()
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

void coms_write(const char *buf, int n)
{
    if (fd < 0)
    {
        fd = open(dev, O_RDWR);
    }

    if (fd > 0)
    {
        int res = write(fd, buf, n);
        if (res != n)
        {
            close(fd);
            fd = -1;
            sleep(2);
        }
    }
}

void coms_dump_hex(const unsigned char *buf, int n)
{
    char str[256];

    for (int i = 0; i < n; i++)
    {
        sprintf(str + 3*i, "%02X ", buf[i]);
    }

    sprintf(str + 3*n, "\n");
    coms_write(str, 3*n+1);
}

void *coms_thread(void *param)
{
    char spare_dev[80];
    char cmd[80];
    int current_dev = atoi(dev + strlen(dev)-1);

    while (1)
    {
        struct stat tmp;
        if (stat(dev, &tmp) == 0)
        {
            sprintf(spare_dev, "/dev/rfcomm%i", ++current_dev);
            DBG("rfcomm device is in use, changing to %s\n", spare_dev);
            dev = (const char *) spare_dev;
            continue;
        }

        sprintf(cmd, "rfcomm listen %s", dev);
        int res = system(cmd);
        DBG("rfcomm signalled to disconnect: %i\n", res);
        if (res != 0)
        {
            DBG("rfcomm exit, hit Ctrl+C again to exit main\n");
            break;
        }
        sleep(3);
    }

    return 0;
}

void coms_init(const char *bt_dev)
{
    dev = bt_dev;
    pthread_create(&rfcomm, NULL, coms_thread, NULL);
}

#ifdef __HAS_MAIN__

int main(void)
{
    coms_init();

    while (1)
    {
        //coms_listen();
        coms_write("moimoi\n", 7);
        sleep(1);
    }

    return 0;
}

#endif


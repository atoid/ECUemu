#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

#include "ecu.h"

#define ECU_BAUD_RATE       10400
#define MAX_READ_TIMEOUT    1000

#define WAIT_BEFORE_PULSE   250
#define WAIT_PULSE          70
#define WAIT_AFTER_PULSE    130
#define WAIT_AFTER_WAKEUP   50

const unsigned char REQ_WAKEUP[]     = { 0xFE, 0x04, 0xFF, 0xFF };
const unsigned char REQ_INITIALIZE[] = { 0x72, 0x05, 0x00, 0xF0, 0x99 };
//const char REQ_READ_TABLE[]  = { 0x72, 0x07, 0x72, 0x11, 0x00, 0x14, 0xF0 };
const unsigned char REQ_READ_TABLE_0A[] = { 0x72, 0x05, 0x71, 0x0a, 0x0e };
const unsigned char REQ_READ_TABLE_10[] = { 0x72, 0x05, 0x71, 0x10, 0x08 };
const unsigned char REQ_READ_TABLE_11[] = { 0x72, 0x05, 0x71, 0x11, 0x07 };
const unsigned char REQ_READ_TABLE_20[] = { 0x72, 0x05, 0x71, 0x20, 0xf8 }; 

const unsigned char RESP_INITIALIZE[] = { 0x02, 0x04, 0x00, 0xFA };

static int fd = -1;
static int fake_writes = 0;
static int custom_baud = 0;
static int table_nbr = -1;

static unsigned char REQ_READ_TABLE_XX[] = { 0x72, 0x05, 0x71, 0x00, 0x00 }; 

void dump_buf(const unsigned char *buf, int n, const char *msg)
{
    if (msg)
    {
        DBG("%s\n", msg);
    }

    DBG("length %i bytes\n", n);

    for (int i = 0; i < n; i++)
    {
        DBG(" %02X", buf[i]);
    }
    DBG("\n");
}

void port_flush(void)
{
    if (tcflush(fd, TCIFLUSH) < 0)
    {
        perror("TCFLUSH");
    }
}

int port_fake_input(const unsigned char *buf, int n)
{
    for (int i = 0; i < n; i++)
    {
        char fake_c = buf[i];
        ioctl(fd, TIOCSTI, &fake_c);
    }

    return n;
}

int port_init(void)
{
    struct termios tio;
    tcgetattr(fd, &tio);
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_lflag = 0;
    //cfsetspeed(&tio, B38400);
    cfsetspeed(&tio, B9600);
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &tio) < 0)
    {
        perror("TCSETATTR");
        return -1;
    }

    if (custom_baud)
    {
        struct serial_struct ser_info;
        if (ioctl(fd, TIOCGSERIAL, &ser_info) < 0)
        {
            perror("TIOCGSERIAL");
            return -1;
        }

        ser_info.flags &= ~ASYNC_SPD_MASK;
        ser_info.flags |= ASYNC_SPD_CUST;
        ser_info.custom_divisor = (ser_info.baud_base / custom_baud);

        if (ioctl(fd, TIOCSSERIAL, &ser_info) < 0)
        {
            perror("TIOCSSERIAL");
            return -1;
        }
    }

    port_flush();
    return 0;
}

int port_set_break(int on)
{
    int res = 0;

    if (on)
    {
        res = ioctl(fd, TIOCSBRK, NULL);
    }
    else
    {
        res = ioctl(fd, TIOCCBRK, NULL);
    }

    if (res < 0)
    {
        perror("TIOCS/CBRK");
    }

    return res;
}

void msleep(int msec)
{
    usleep(msec * 1000);
}

int ecu_get_csum(const unsigned char *buf, int n)
{
    int csum = 0;
    for (int i = 0; i < n-1; i++)
    {
        csum += buf[i];
    }
    csum = (0x100 - csum) & 0xff;
    return csum;
}

int ecu_verify_msg(const unsigned char *buf, int n)
{
    int csum = ecu_get_csum(buf, n);

    if (n != buf[1] || csum != buf[n-1])
    {
        dump_buf(buf, n, "!!message");
        if (n != buf[1])
            DBG("!!length mismatch %02X should be %02X\n", buf[1], n);
        if (csum != buf[n-1])
            DBG("!!checksum mismatch %02X should be %02X\n", buf[n-1], csum);
        return -1;
    }

    return csum;
}

int ecu_write(const unsigned char *buf, int n)
{
    if (ecu_verify_msg(buf, n) < 0)
    {
        return -1;
    }

    if (fake_writes)
    {
        return port_fake_input(buf, n);
    }
    else
    {
        return write(fd, buf, n);
    }
}

int ecu_read(unsigned char *buf, int n)
{
    int i = 0;
    int total_wait = 0;
    int to_read = n;

    while (i < n && total_wait < MAX_READ_TIMEOUT)
    {
        int nread = read(fd, buf+i, to_read);

        if (nread > 0)
        {
            i += nread;
            to_read -= nread;
        }
        else
        {
            msleep(5);
            total_wait += 5;
        }
    }

    return i;
}

int ecu_init(void)
{
    DBG("ecu initialize\n");

    msleep(WAIT_BEFORE_PULSE);
    port_set_break(1);
    msleep(WAIT_PULSE);
    port_set_break(0);
    msleep(WAIT_AFTER_PULSE);
    port_flush();

    ecu_write(REQ_WAKEUP, sizeof(REQ_WAKEUP));
    msleep(WAIT_AFTER_WAKEUP);

    ecu_write(REQ_INITIALIZE, sizeof(REQ_INITIALIZE));

    unsigned char buf[32];
    int n = ecu_read(buf, sizeof(buf));
    dump_buf(buf, n, "ecu response");
    return 0;
}

void usage(void)
{
    printf("usage: ecu [options]\n");
    printf("options:\n");
    printf(" -d <device> e.g. /dev/ttyUSB0\n");
    printf(" -b <custom baud>\n");
    printf(" -r <table nbr> read table\n");
    printf(" -f fake writes\n");
    printf(" -h usage\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    const char *dev = "/dev/ttyUSB0";
    int opt;

    while( (opt = getopt( argc, argv, "fhb:d:r:" )) != -1 ) {
        switch( opt ) {
            case 'd':
                dev = optarg;
                break;
            case 'b':
                custom_baud = atoi(optarg);
                if (custom_baud == 0)
                {
                    custom_baud = ECU_BAUD_RATE;
                }
                printf("set %i custom baud\n", custom_baud);
                break;
            case 'h':
                usage();
                break;
            case 'f':
                printf("fake writes (device loopback)\n");
                fake_writes = 1;
                break;
            case 'r':
                table_nbr = atoi(optarg);
                break;
            default:
                break;
        }
    }

    fd = open(dev, O_RDWR | O_NOCTTY);

    if (fd > 0)
    {
        if (port_init() < 0)
        {
            perror("port init");
            return -1;
        }

        ecu_init();

        if (table_nbr != -1)
        {
            printf("read table %02X\n", table_nbr);
            REQ_READ_TABLE_XX[3] = table_nbr;
            REQ_READ_TABLE_XX[4] = ecu_get_csum(REQ_READ_TABLE_XX, sizeof(REQ_READ_TABLE_XX));
            ecu_write(REQ_READ_TABLE_XX, sizeof(REQ_READ_TABLE_XX));
            unsigned char buf[32];
            int n = ecu_read(buf, sizeof(buf));
            ecu_parse_table(buf, n);
        }

        msleep(200);
        close(fd);
    }
    else
    {
        perror("port open");
        return -1;
    }

    return 0;
}


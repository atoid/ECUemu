#include <stdio.h>
#include <stdlib.h>

#include "ecu.h"

static const unsigned char RESP_TABLE_10[] = {
    0x10, 0x80, // RPM
    0x18, // TPS v
    0x00, // TPS %
    0x37, // ECT v
    0x70, // ECT deg
    0x56, // IAT v
    0x5c, // IAT deg
    0x87, // MAP v
    0x5c, // MAP kPa
    0xff, // res
    0xff, // res
    0x7a, // vbatt
    0x00, // speed
    0x00, 0x00, // fuel injection
    0x80  // res
};


float ecu_data_to_volts(int v)
{
    return (float)v * 5.f / 256.f;
}

int ecu_data_to_deg(int v)
{
    return v - 40;
}

float ecu_data_to_vbat(int v)
{
    return (float)v / 10.f;
}

int ecu_data_to_rpm(int v1, int v2)
{
    return v1 * 256 + v2;
}

void ecu_parse_table(unsigned char *buf, int n)
{
    dump_buf(buf, n, "table data");

    const unsigned char *resp = RESP_TABLE_10;

    //DBG("SPD  : %i kph\n", resp[13]);
    //DBG("RPM  : %i\n", ecu_data_to_rpm(resp[0], resp[1]));
    //DBG("ECT  : %i deg C\n", ecu_data_to_deg(resp[5]));
    //DBG("IAT  : %i deg C\n", ecu_data_to_deg(resp[7]));
    //DBG("VBAT : %.1f V\n", ecu_data_to_vbat(resp[12]));
}


#ifndef ECU_H
#define ECU_H

#define DBG(...) printf(__VA_ARGS__)

/* ecu.c */
void dump_buf(const unsigned char *buf, int n, const char *msg);

/* ecu_tables.c */
float ecu_data_to_volts(int v);
int ecu_data_to_deg(int v);
float ecu_data_to_vbat(int v);
int ecu_data_to_rpm(int v1, int v2);
void ecu_parse_table(unsigned char *buf, int n);

/* ecu_coms.c */
void coms_listen(const char *dev);
void coms_write(const char *dev, const char *data, int n);

#endif

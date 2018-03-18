ecu: ecu.c ecu_tables.c ecu_coms.c ecu.h
	gcc -Wall -lpthread ecu.c ecu_tables.c ecu_coms.c -o ecu

clean:
	rm ecu


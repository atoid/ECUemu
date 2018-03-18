ecu: ecu.c ecu_tables.c ecu_coms.c ecu.h
	gcc -Wall ecu.c ecu_tables.c ecu_coms.c -o ecu
	gcc -Wall -D__HAS_MAIN__ ecu_coms.c -o ecu_coms

clean:
	rm ecu
	rm ecu_coms


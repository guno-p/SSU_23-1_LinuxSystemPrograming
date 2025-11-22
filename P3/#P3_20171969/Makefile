ssu_monitor : main.o ssu_monitor.o
	gcc main.o ssu_monitor.o -o ssu_monitor

main.o : main.c ssu_monitor.h
	gcc -c main.c

ssu_monitor.o : ssu_monitor.c ssu_monitor.h
	gcc -c ssu_monitor.c

clean :
	rm *.o
	rm ssu_monitor
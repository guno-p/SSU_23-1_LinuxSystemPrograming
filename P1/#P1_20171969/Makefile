CC = gcc

all: ssu_backup help add remove recover

ssu_backup: ssu_backup.c
	$(CC) $^ -I/usr/local/opt/openssl@3/include -L/usr/local/opt/openssl@3/lib -lcrypto -o $@
# to link....~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ is only my enviromental action
add: add.c
	$(CC) $^ -I/usr/local/opt/openssl@3/include -L/usr/local/opt/openssl@3/lib -lcrypto -o $@

remove: remove.c
	$(CC) $^ -I/usr/local/opt/openssl@3/include -L/usr/local/opt/openssl@3/lib -lcrypto -o $@

recover: recover.c
	$(CC) $^ -I/usr/local/opt/openssl@3/include -L/usr/local/opt/openssl@3/lib -lcrypto -o $@

help: help.c
	$(CC) $^ -I/usr/local/opt/openssl@3/include -L/usr/local/opt/openssl@3/lib -lcrypto -o $@

clean:
	rm ssu_backup add remove recover help
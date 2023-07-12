compile:
	gcc -Wall -g3 -fsanitize=address -pthread servidor.c -o server
	gcc -Wall -g3 -fsanitize=address -pthread cliente.c -o client
FLAGS    = -L /lib64
LIBS     = -lusb-1.0 -l pthread

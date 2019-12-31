all: server client

CC = clang++

OPTIONS = -Wall -O3 -std=c++17

server: server.cpp server.h
	$(CC) $(OPTIONS) -o server server.cpp

client: client.cpp client.h
	$(CC) $(OPTIONS) -o client client.cpp

clean:
	-rm -f {*.o,server,client}
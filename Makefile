CC=g++ -std=c++11 -Wall -Wextra

build: server subscriber

server.o: server.c
	$(CC) $^ -c

subscriber.o: subscriber.c
	$(CC) $^ -c

run_server: 
	./server 3030


run_subscriber:
	./subscriber 1234 127.0.0.1 3030

clean:
	rm *.o server subscriber
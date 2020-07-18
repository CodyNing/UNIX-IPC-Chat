CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror

all: build

build:
	gcc $(CFLAGS) syncList.c controller.c sender.c inputHandler.c printer.c reciever.c main.c -lpthread -o s-talk instructorList.o


run: build
	./s-talk 22111 127.0.0.1 22110

valgrind: build
	valgrind --leak-check=full --show-leak-kinds=all ./s-talk 22110 127.0.0.1 22111

valgrindin: build
	cat in.txt | valgrind --leak-check=full --show-leak-kinds=all ./s-talk 22111 127.0.0.1 22110

clean:
	rm -f s-talk

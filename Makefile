CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror

all: build

build:
	gcc $(CFLAGS) syncList.c controller.c sender.c inputHandler.c printer.c reciever.c main.c -lpthread -o main instructorList.o


run: build
	./main

valgrind: build
	valgrind --leak-check=full ./main

clean:
	rm -f main

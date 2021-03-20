
all: build

build: main.o map.o utils.o
	gcc -o so-cpp main.o map.o utils.o

main.o: main.c
	gcc -Wall -c main.c

map.o: map.c
	gcc -Wall -c map.c

utils.o: utils.c
	gcc -Wall -c utils.c

.PHONY: clean
clean:
	rm -f main.o map.o utils.o so-cpp

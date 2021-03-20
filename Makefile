all: build

build: main.o map.o utils.o
	cl /Feso-cpp.exe main.o map.o utils.o /MD

main.o: main.c
	cl /c main.c /MD

map.o: map.c
	cl /c map.c /MD

utils.o: utils.c
	cl /c utils.c /MD

.PHONY: clean
clean:
	del /Q /F so-cpp.exe main.o map.o utils.o

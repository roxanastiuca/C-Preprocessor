all: build

build: main.obj map.obj utils.obj
	cl /Feso-cpp.exe main.obj map.obj utils.obj /MD

main.obj: main.c
	cl /c main.c /MD

map.obj: map.c
	cl /c map.c /MD

utils.obj: utils.c
	cl /c utils.c /MD

.PHONY: clean
clean:
	del /Q /F so-cpp.exe main.o map.o utils.o

all: build

build:
	cl /Feso-cpp.exe main.c map.c utils.c /MD

.PHONY: clean

clean:
	del /Q /F so-cpp.exe

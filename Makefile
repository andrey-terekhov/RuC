# Legacy Makefile taking advantage of CMake script
.PHONY: build

all: build

build:
	mkdir -p build && cd build && cmake .. && make

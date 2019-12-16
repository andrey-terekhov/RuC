# Legacy Makefile taking advantage of CMake script
.PHONY: build

all: build

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

cmake_with_ninja := $(shell command -v ninja > /dev/null && echo -GNinja || echo "")
build_command := $(shell command -v ninja > /dev/null && echo ninja || echo make)
build_dir := $(shell test $(mkfile_dir) -ef $(shell pwd) && echo $(shell pwd)/build || echo $(shell pwd))
build:
	mkdir -p $(build_dir) && \
	cd $(build_dir) && \
	cmake $(cmake_with_ninja) $(mkfile_dir) && \
	$(build_command)

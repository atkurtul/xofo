#!/usr/bin/bash

all:
	mkdir -p build
	cmake -S . -B ./build -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build ./build
	./build/xofo

clean:
	rm -rf ./build


run: all
	@build/vk
	
all:
	@mkdir -p build
	@cmake -S . -B build -G Ninja
	@cd build && ninja

clean:
	rm -rf build 
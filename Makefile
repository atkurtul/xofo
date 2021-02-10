
run: all
	@build/vk
	
all:
	@mkdir -p build
	@cmake -S . -B build -G Ninja
	@cd build && ninja
	@mv build/compile_commands.json .
	
clean:
	rm -rf build
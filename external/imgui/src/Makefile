SRC_FILES := $(wildcard ./*.cpp)
OBJ_FILES := $(patsubst ./%.cpp,./%.o,$(SRC_FILES))
CXXFLAGS := -I../include

all: $(OBJ_FILES)
	g++ -o main $^ -lvulkan -lglfw

%.o: %.cpp
	g++ $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm $(OBJ_FILES) main imgui.ini
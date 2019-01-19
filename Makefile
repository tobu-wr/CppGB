OBJS = Source/*.cpp Source/*.h
COMPILER = g++
COMPILER_FLAGS = -std=c++17
LINKER_FLAGS = `sdl2-config --cflags --libs`

build: $(OBJS)
	$(COMPILER) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o CppGB

.PHONY: release
release: COMPILER_FLAGS += -O2 -march=native
release: build

.PHONY: clean
clean:
	rm CppGB

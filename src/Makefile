FLAGS = -O3 -Wall -Wextra -shared -std=c++11 -fPIC

INCLUDES = -I../include

PY_INCLUDES := $(shell python3-config --includes)

LINKER_OPTIONS = -Wl,-undefined,dynamic_lookup

OUT := $(shell python3-config --extension-suffix)


.PHONY: snakefish clean

snakefish: snakefish.cpp thread.cpp channel.cpp
	g++ $(FLAGS) $(INCLUDES) $(PY_INCLUDES) $(LINKER_OPTIONS) $^ -o c$@$(OUT)
	mv csnakefish.*.so ./snakefish

clean:
	rm -rf ./csnakefish.*.so

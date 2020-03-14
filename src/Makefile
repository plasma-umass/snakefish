FLAGS = -O3 -Wall -Wextra -shared -std=c++11 -fPIC

INCLUDES = -I../include

PY_INCLUDES := $(shell python3-config --includes)

LINKER_OPTIONS = -Wl,-undefined,dynamic_lookup

OUT := $(shell python3-config --extension-suffix)

SRC = buffer.cpp channel.cpp shared_buffer.cpp snakefish.cpp thread.cpp


.PHONY: snakefish clean

snakefish: $(SRC)
	g++ $(FLAGS) $(INCLUDES) $(PY_INCLUDES) $(LINKER_OPTIONS) $^ -o c$@$(OUT)
	cp csnakefish.*.so ./snakefish

clean:
	rm -rf ./csnakefish.*.so

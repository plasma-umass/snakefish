FLAGS = -O3 -Wall -Wextra -shared -std=c++11 -fPIC -fvisibility=hidden

INCLUDES = -I../include

PY_INCLUDES := $(shell python3-config --includes)

LINKER_OPTIONS = -Wl,-undefined,dynamic_lookup

OUT := $(shell python3-config --extension-suffix)

SRC = buffer.cpp channel.cpp generator.cpp semaphore_t.cpp snakefish.cpp thread.cpp


.PHONY: snakefish clean

snakefish: $(SRC)
	g++ $(FLAGS) $(INCLUDES) $(PY_INCLUDES) $(LINKER_OPTIONS) $^ -o $@$(OUT)

clean:
	rm -rf ./*$(OUT)

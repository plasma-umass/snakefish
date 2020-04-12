OUT := $(shell python3-config --extension-suffix)

.PHONY: snakefish

snakefish:
	$(MAKE) -C ../src
	cp ../src/*$(OUT) .

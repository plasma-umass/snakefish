.PHONY: snakefish pybind11 dev_dependency

snakefish: pybind11
	$(MAKE) -C src

pybind11:
	@mkdir -p include && \
	mkdir -p temp && \
	cd temp && \
	git clone --recursive 'https://github.com/pybind/pybind11.git' && \
	cd pybind11 && \
	cp -r ./include/* ../../include/ && \
	cd ../.. && \
	rm -rf temp

# dependencies to build tests
dev_dependency:
	@rm -rf pybind11 googletest
	@git clone --recursive 'https://github.com/pybind/pybind11.git'
	@git clone --recursive 'https://github.com/google/googletest.git'

.PHONY: snakefish pybind11 pip dev_dependency

snakefish: pybind11 pybind11 pip
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

pip:
	pip3 install --user pybind11
	pip3 install --user wrapt
	pip3 install --user numpy
	pip3 install --user numexpr
	pip3 install --user OpenCV-python

# dependencies to build tests
dev_dependency:
	@rm -rf pybind11 googletest
	@git clone --recursive 'https://github.com/pybind/pybind11.git'
	@git clone --recursive 'https://github.com/google/googletest.git'

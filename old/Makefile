

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
ABS_PATH := $(shell dirname $(mkfile_path))

build:: pybind11 pip

pybind11:
	@mkdir -p $(ABS_PATH)/src/lib $(ABS_PATH)/src/include && \
	mkdir -p $(ABS_PATH)/temp && cd $(ABS_PATH)/temp && \
	git clone --recursive 'https://github.com/pybind/pybind11.git' && \
	cd pybind11 && cp -r ./include/* ../../src/include/ && \
	cd $(ABS_PATH) && rm -rf $(ABS_PATH)/temp

pip:
	pip3 install --user pybind11
	pip3 install --user wrapt
	pip3 install --user numpy
	pip3 install --user numexpr
	pip3 install --user OpenCV-python

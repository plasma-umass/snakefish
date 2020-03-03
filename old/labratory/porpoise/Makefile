
CXX := g++
CXXLIB := $(CXX) -shared -fPIC -Wl,-soname,interposer.so

all:: cporpoise.cc dynamics/logger.so
	g++ -O3 -Wall -I../include `python3-config --includes` -shared -std=c++11 -fPIC cporpoise.cc -o cporpoise`python3-config --extension-suffix`

dynamics/logger.so: dynamics/logger.cc
	$(CXXLIB) -I../include --std=c++11 -o $@ $< -ldl

clean:
	rm -rf ./cporpoise.*.so ./dynamics/logger.so

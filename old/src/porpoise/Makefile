
CXX := g++
CXXLIB := $(CXX) -shared -fPIC -Wl,-soname,interposer.so

all:: cporpoise.cc
	g++ -O3 -Wall -I../include `python3-config --includes` -shared -std=c++11 -fPIC cporpoise.cc -o cporpoise`python3-config --extension-suffix`

clean:
	rm -rf ./cporpoise.*.so

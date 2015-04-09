CC = g++-4.8
SRC = TestRegex.cpp
LIBS = -L../lib -lcontextgraph -L boost_regex
OBJ = $(SRC:.cpp=.o)
OUT = test
BOOSTLIB = -L/home/adrian/boost-trunk/bin.v2/libs/regex/build/gcc-4.8/release/link-static/threading-multi -lboost_regex
BOOST = ../../../boost-trunk/
INCLUDES = -I. -I../include/ -I../ContextGraph/  -I$(BOOST)
CCFLAGS = -g -Wall -pedantic -std=c++11 -DTESTING
LDFLAGS = -g

all : $(OUT)

parent: 
	cd ../ContextGraph; make clean; make; cd ../TestContextGraph; make clean; make

.cpp.o:
	$(CC) -c $(CCFLAGS) $(INCLUDES) $<

$(OUT): $(OBJ)
	$(CC) -o $(OUT) $(LDFLAGS) $(OBJ) $(LIBS) $(BOOSTLIB)

clean:
	rm -rf *.o
	rm -rf $(OUT)

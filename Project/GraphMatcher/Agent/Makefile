CC = g++
SRC = Agent.cpp
LIBS = -L../lib -lcontextgraph -Lboost_regex
OBJ = $(SRC:.cpp=.o)
OUT = agent
BOOSTLIB = -L/home/adrian/boost-trunk/bin.v2/libs/regex/build/gcc-4.8/release/link-static/threading-multi -lboost_regex
BOOST = ../../../boost-trunk/
INCLUDES = -I. -I../include -I../ContextGraph -I$(BOOST)
CCFLAGS = -g -Wall -pedantic -std=c++11
LDFLAGS = -g

all : $(OUT)

.cpp.o:
	$(CC) -c $(CCFLAGS) $(INCLUDES) $<

$(OUT) : $(OBJ)
	$(CC) -o $(OUT) $(LDFLAGS) $(OBJ) $(LIBS) $(BOOSTLIB)

clean:
	rm -rf *.o
	rm -rf $(OUT)

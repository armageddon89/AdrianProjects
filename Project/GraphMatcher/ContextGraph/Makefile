CC = g++-4.8
SRC = ContextGraph.cpp
LIBOUT = ../lib
OBJ = $(SRC:.cpp=.o)
OUT = libcontextgraph.a
BOOST = ../../../boost-trunk/
INCLUDES = -I. -I../include/ -I$(BOOST)
CCFLAGS = -g -Wall -pedantic -std=c++11

all : $(OUT)

.cpp.o:
	$(CC) -c $(CCFLAGS) $(INCLUDES)  $<

$(OUT): $(OBJ)
	ar -rv $(LIBOUT)/$(OUT) $(OBJ)
clean:
	rm -rf *.o $(LIBOUT)/*$(OUT)

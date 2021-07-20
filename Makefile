include ../../build-unix/Makefile.config

PROJECT := simple_pingpong
SRCS    := $(wildcard *.cpp)
OBJS    := $(SRCS:.cpp=.o)

include ../../build-unix/Makefile.rules

BINARY := conway

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

CXX := g++
CXXFLAGS := -std=c++20 -g -Wall -Werror -pedantic -MMD -MP
LDFLAGS :=
LDLIBS := -lsfml-graphics -lsfml-window -lsfml-system

.PHONY: clean

all: $(BINARY)

clean:
	$(RM) $(OBJS) $(DEPS) $(BINARY)

$(BINARY): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

-include $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

BUILD ?= release

BINARY := conway

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

CXX := g++
CXXFLAGS.debug = -g3 -Og -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
CXXFLAGS.release = -O3 -g -march=native -DNDEBUG
CXXFLAGS := -std=c++20 $(CXXFLAGS.$(BUILD)) -Wall -Wextra -Werror -pedantic -MMD -MP
LDFLAGS.debug = -fsanitize=address,undefined
LDFLAGS.release =
LDFLAGS := $(LDFLAGS.$(BUILD))
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

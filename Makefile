BUILD ?= release

SRCDIR := src
INCDIR := include
BLDDIR := build
OBJDIR := $(BLDDIR)/$(BUILD)
BINARY := conway

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPS := $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d)

CXX := g++
CXXFLAGS.debug = -g3 -Og -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
CXXFLAGS.release = -O3 -g -march=native -DNDEBUG
CXXFLAGS := -std=c++20 $(CXXFLAGS.$(BUILD)) -Wall -Wextra -Werror -pedantic -Wconversion -Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast -MMD -MP
LDFLAGS.debug = -fsanitize=address,undefined
LDFLAGS.release =
LDFLAGS := $(LDFLAGS.$(BUILD))
LDLIBS := -lsfml-graphics -lsfml-window -lsfml-system

.PHONY: all clean compiledb $(BINARY)

all: $(BINARY)

clean:
	$(RM) -r $(BLDDIR) $(BINARY)

check:
	run-clang-tidy $(SRCS)

compiledb: clean
	bear -- $(MAKE) BUILD=debug -j

$(BINARY): $(OBJDIR)/$(BINARY)
	ln -sf $< $@

$(OBJDIR)/$(BINARY): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

-include $(DEPS)

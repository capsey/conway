BUILD ?= release
ARCH ?= native

LINTER ?= clang-tidy
BEAR ?= bear
LN ?= ln -sf
MKDIR ?= mkdir -p

SRCDIR := src
INCDIR := include
BLDDIR := build
OBJDIR := $(BLDDIR)/$(BUILD)
BINARY := conway

HDRS := $(wildcard $(INCDIR)/*.hpp)
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPS := $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.d)

CXX ?= g++
CXXFLAGS.debug = -g3 -Og -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
CXXFLAGS.release = -O3 -g -march=$(ARCH) -DNDEBUG
CXXFLAGS := -std=c++20 $(CXXFLAGS.$(BUILD)) -Wall -Wextra -Werror -pedantic -Wconversion -Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast -MMD -MP
LDFLAGS.debug = -fsanitize=address,undefined
LDFLAGS.release =
LDFLAGS := $(LDFLAGS.$(BUILD))
LDLIBS := -lsfml-graphics -lsfml-window -lsfml-system

.PHONY: all clean check compiledb $(BINARY)

all: $(BINARY)

clean:
	$(RM) -r $(BLDDIR) $(BINARY)

check: compile_commands.json
	$(LINTER) $(SRCS) $(HDRS)

compile_commands.json:
	$(BEAR) -- $(MAKE) --always-make

$(BINARY): $(OBJDIR)/$(BINARY)
	$(LN) $< $@

$(OBJDIR)/$(BINARY): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	$(MKDIR) $(OBJDIR)

-include $(DEPS)

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter
LDLIBS   := -ldl -lm -lpthread
TARGET   := furious_swine

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:src/%.cpp=build/obj/%.o)
DEPS := $(OBJS:.o=.d) build/obj/tools/level_lab.d

.PHONY: all run test clean tools

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDLIBS)

build/obj/%.o: src/%.cpp | build/obj
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/obj:
	mkdir -p $@

# Developer tools (not built by default): make tools
GAME_OBJS := $(filter-out build/obj/main.o,$(OBJS))

tools: level_lab

level_lab: build/obj/tools/level_lab.o $(GAME_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

build/obj/tools/%.o: tools/%.cpp | build/obj
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Isrc -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) --self-test

clean:
	rm -rf build/obj $(TARGET) level_lab

-include $(DEPS)

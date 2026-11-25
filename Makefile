CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter
LDLIBS   := -ldl -lm -lpthread
TARGET   := furious_swine

SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:src/%.cpp=build/obj/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDLIBS)

build/obj/%.o: src/%.cpp | build/obj
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/obj:
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) --self-test

clean:
	rm -rf build/obj $(TARGET)

-include $(DEPS)

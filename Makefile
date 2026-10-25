CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter
TARGET   := furious_swine
SRC      := src/furious_swine.cpp
LDFLAGS  := -ldl -lm -lpthread

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) --self-test

clean:
	rm -f $(TARGET)

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS := -pthread

SRC_DIR := src
BUILD_DIR := build
TARGET := kvstore

SOURCES := $(SRC_DIR)/main.cpp
OBJECTS := $(BUILD_DIR)/main.o

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(SRC_DIR)/*.hpp
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/main.o

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

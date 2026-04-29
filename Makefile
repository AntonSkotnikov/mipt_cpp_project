CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -g
CPPFLAGS = -ICommon/Inc -IClient/Inc -IServer/Inc

BUILD_DIR = build

COMMON_SRC = $(filter-out Common/Src/client.cpp,$(wildcard Common/Src/*.cpp))
CLIENT_SRC = $(wildcard Client/Src/*.cpp)
SERVER_SRC = $(wildcard Server/Src/*.cpp)

CLIENT_TARGET = $(BUILD_DIR)/client
SERVER_TARGET = $(BUILD_DIR)/server

.PHONY: all clean

all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(COMMON_SRC) $(CLIENT_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@ -lncurses

$(SERVER_TARGET): $(COMMON_SRC) $(SERVER_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)

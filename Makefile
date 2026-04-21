CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -g

INCLUDES = -ICommon/Inc -IClient/Inc -IInterface/Inc -IServer/Inc -lncurses

BUILD_DIR = build


COMMON_SRC    = $(wildcard Common/Src/*.cpp)
CLIENT_SRC    = $(wildcard Client/Src/*.cpp)
INTERFACE_SRC = $(wildcard Interface/Src/*.cpp)
SERVER_SRC    = $(wildcard Server/Src/*.cpp)

CLIENT_TARGET = $(BUILD_DIR)/client
#SERVER_TARGET = $(BUILD_DIR)/server

all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(COMMON_SRC) $(CLIENT_SRC) $(INTERFACE_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

#$(SERVER_TARGET): $(COMMON_SRC) $(SERVER_SRC)
#	mkdir -p $(BUILD_DIR)
#	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)

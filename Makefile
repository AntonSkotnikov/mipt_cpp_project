# CXX = g++
# CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -g
#
# INCLUDES = -ICommon/Inc -IClient/Inc -IInterface/Inc -IServer/Inc -lncurses
#
# BUILD_DIR = build
#
#
# COMMON_SRC    = $(wildcard Common/Src/*.cpp)
# CLIENT_SRC    = $(wildcard Client/Src/*.cpp)
# INTERFACE_SRC = $(wildcard Interface/Src/*.cpp)
# SERVER_SRC    = $(wildcard Server/Src/*.cpp)
#
# CLIENT_TARGET = $(BUILD_DIR)/client
# SERVER_TARGET = $(BUILD_DIR)/server
#
# all: $(CLIENT_TARGET) $(SERVER_TARGET)
#
# $(CLIENT_TARGET): $(COMMON_SRC) $(CLIENT_SRC) $(INTERFACE_SRC)
# 	mkdir -p $(BUILD_DIR)
# 	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@
#
# $(SERVER_TARGET): $(COMMON_SRC) $(SERVER_SRC)
# 	mkdir -p $(BUILD_DIR)
# 	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@
#
# clean:
# 	rm -rf $(BUILD_DIR)

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -g
CPPFLAGS = -ICommon/Inc -IClient/Inc -IInterface/Inc -IServer/Inc

CLIENT_LDLIBS = -lncurses
CLIENT_STUB_LDLIBS =
SERVER_LDLIBS =

BUILD_DIR = build

COMMON_SRC    = $(wildcard Common/Src/*.cpp)
CLIENT_SRC    = $(filter-out Client/Src/main.cpp Client/Src/ConsoleUi.cpp Client/Src/DummyTransport.cpp,$(wildcard Client/Src/*.cpp))
CLIENT_STUB_SRC = $(COMMON_SRC) $(CLIENT_SRC) Client/Src/ConsoleUi.cpp Client/Src/DummyTransport.cpp Client/Src/main.cpp

INTERFACE_SRC = $(wildcard Interface/Src/*.cpp)
SERVER_SRC    = $(wildcard Server/Src/*.cpp)

CLIENT_TARGET      = $(BUILD_DIR)/client
CLIENT_STUB_TARGET = $(BUILD_DIR)/client_stub
SERVER_TARGET      = $(BUILD_DIR)/server

all: $(CLIENT_STUB_TARGET)

full: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_STUB_TARGET): $(CLIENT_STUB_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@ $(CLIENT_STUB_LDLIBS)

$(CLIENT_TARGET): $(COMMON_SRC) $(CLIENT_SRC) $(INTERFACE_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@ $(CLIENT_LDLIBS)

$(SERVER_TARGET): $(COMMON_SRC) $(SERVER_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@ $(SERVER_LDLIBS)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all full clean

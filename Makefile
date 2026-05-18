DEBUGGAME = 0

ifeq ($(DEBUGGAME), 1)
	DEFINES = -D DEBUGGAME
else
	DEFINES = 
endif

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -g
CPPFLAGS = -ICommon/Inc -IClient/Inc -IInterface/Inc -IServer/Inc
PKG_CONFIG ?= pkg-config

NCURSES_PKG := $(shell if command -v $(PKG_CONFIG) >/dev/null 2>&1; then \
	for pkg in ncursesw ncurses curses; do \
		if $(PKG_CONFIG) --exists $$pkg; then \
			printf '%s\n' $$pkg; \
			break; \
		fi; \
	done; \
fi)

ifneq ($(NCURSES_PKG),)
NCURSES_CPPFLAGS ?= $(shell $(PKG_CONFIG) --cflags $(NCURSES_PKG))
NCURSES_LIBS ?= $(shell $(PKG_CONFIG) --libs $(NCURSES_PKG))
else
NCURSES_CPPFLAGS ?=
NCURSES_LIBS ?= $(shell tmp=$$(mktemp /tmp/plague-ncurses.XXXXXX); \
	trap 'rm -f "$$tmp" "$$tmp.cpp"' EXIT; \
	printf 'int main(){return 0;}\n' > "$$tmp.cpp"; \
	for libs in "-lncursesw" "-lncurses" "-lcurses"; do \
		if $(CXX) "$$tmp.cpp" -o "$$tmp" $$libs >/dev/null 2>&1; then \
			printf '%s\n' "$$libs"; \
			exit 0; \
		fi; \
	done; \
	printf '%s\n' "-lncurses"; \
)
endif

BUILD_DIR = build

COMMON_SRC = $(filter-out Common/Src/client.cpp,$(wildcard Common/Src/*.cpp))
CLIENT_SRC = $(wildcard Client/Src/*.cpp)
INTERFACE_SRC = $(wildcard Interface/Src/*.cpp)
SERVER_SRC = $(wildcard Server/Src/*.cpp)

CLIENT_TARGET = $(BUILD_DIR)/client
SERVER_TARGET = $(BUILD_DIR)/server

.PHONY: all clean

all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(COMMON_SRC) $(CLIENT_SRC) $(INTERFACE_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(NCURSES_CPPFLAGS) $(DEFINES) $^ -o $@ $(NCURSES_LIBS)

$(SERVER_TARGET): $(COMMON_SRC) $(SERVER_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)

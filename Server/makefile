# Компилятор и флаги
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Iinc
LDFLAGS  = -luv
TARGET   = plague_server

# Список исходников и объектных файлов
SRCS = Src/main_server.cpp Src/PlagueServer.cpp
OBJS = $(SRCS:.cpp=.o)

# Цель по умолчанию — собрать исполняемый файл
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Правило компиляции .cpp в .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Очистка
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
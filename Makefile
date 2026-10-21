CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

TARGET = codeforge
SRCS = main.cpp editor.cpp buffer.cpp cursor.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

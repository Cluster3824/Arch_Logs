CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = archlog
SRCS = src/main.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)/usr/bin/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
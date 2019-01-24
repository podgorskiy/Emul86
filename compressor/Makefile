TARGET = main
LIBS =
CC = g++
CXX = g++
CFLAGS = -O3 -I "lz4/" -Wall -Wextra -Wundef -Wshadow -Wcast-align
CPPFLAGS = -O3 -std=c++11 -I "lz4/" -Wall -Wextra -Wundef -Wshadow -Wcast-align
LDFLAGS = -flto -m64 -lstdc++

.PHONY: default all clean

default: $(TARGET)
all: default

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

OBJECTS = $(patsubst %.c, %.o, $(call rwildcard, , *.c)) $(patsubst %.cpp, %.o, $(call rwildcard, , *.cpp))
HEADERS = $(wildcard *.h) $(wildcard *.hpp)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

SOURCES=canvas-test.cpp
OBJS=$(SOURCES:.cpp=.o)
TARGET=canvas-test

all: $(TARGET)

canvas-test.o: canvas-test.cpp

%.o: %.cpp
	g++ `pkg-config --cflags gtk+-3.0` -c -o $@ $< `pkg-config --libs gtk+-3.0`

%: %.o
	g++ -o $@ $^ `pkg-config --libs gtk+-3.0`

clean:
	rm -f -- $(OBJS)


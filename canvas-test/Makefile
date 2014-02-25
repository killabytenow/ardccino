SOURCES=canvas-test.cpp UTFT/UTFT.cpp
OBJS=$(SOURCES:.cpp=.o)
TARGET=canvas-test

all: $(TARGET)

canvas-test.o: canvas-test.cpp

UTFT/UTFT.o : UTFT/UTFT.cpp UTFT/UTFT.h UTFT/hardware/gtk/gtk.h

$(TARGET) : $(OBJS)

%.o: %.cpp
	g++ -DSIMULATOR=1 `pkg-config --cflags gtk+-3.0` -c -o $@ $< `pkg-config --libs gtk+-3.0`

%: %.o
	g++ -o $@ $^ `pkg-config --libs gtk+-3.0`

clean:
	rm -f -- $(OBJS)


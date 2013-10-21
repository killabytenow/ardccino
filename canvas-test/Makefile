SOURCES=canvas-test.cpp UTFT/UTFT.cpp
OBJS=$(SOURCES:.cpp=.o)
TARGET=canvas-test

all: $(TARGET)

canvas-test.o: canvas-test.cpp

$(TARGET) : $(OBJS)

%.o: %.cpp
	g++ -DUTFT_GTK_SIMULATION=1 `pkg-config --cflags gtk+-3.0` -c -o $@ $< `pkg-config --libs gtk+-3.0`

%: %.o
	g++ -o $@ $^ `pkg-config --libs gtk+-3.0`

clean:
	rm -f -- $(OBJS)


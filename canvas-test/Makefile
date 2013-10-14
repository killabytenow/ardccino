SOURCES=canvas-test.c
OBJS=$(SOURCES:.c=.o)
TARGET=canvas-test

all: $(TARGET)

canvas-test.o: canvas-test.c

%.o: %.c
	gcc `pkg-config --cflags gtk+-3.0` -c -o $@ $< `pkg-config --libs gtk+-3.0`

%: %.o
	gcc -o $@ $^ `pkg-config --libs gtk+-3.0`

clean:
	rm -f -- $(OBJS)


#ifndef __SERIAL_H__
#define __SERIAL_H__

class StdSerial
{
	public:
		static void begin(int speed);
		static void print(char *str);
		static void print(char c);
		static void println(char *str);
		static void println(unsigned int i);
		static void println(void);
		static int available(void);
		static int read(void);
};

extern StdSerial Serial;

#endif

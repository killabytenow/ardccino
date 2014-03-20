#include <stdio.h>

#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifdef __cplusplus

class StdSerial
{
	public:
		int fd;

		StdSerial();
		void set_fd(int fd);
		void begin(int speed);
		void print(const char *str);
		void print(char c);
		void print(int c);
		void print(unsigned int c);
		void println(const char *str);
		void println(char c);
		void println(int i);
		void println(unsigned int i);
		void println(void);
		int available(void);
		int read(void);
};

extern StdSerial Serial;

#endif

#endif

#ifndef __SERIAL_H__
#define __SERIAL_H__

#ifdef __cplusplus

class StdSerial
{
	public:
		StdSerial();
		void begin(int speed);
		void print(char *str);
		void print(char c);
		void println(char *str);
		void println(unsigned int i);
		void println(void);
		int available(void);
		int read(void);
};

extern StdSerial Serial;

#endif

#endif

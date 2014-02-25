#include "StdSerial.h"
#include <stdio.h>

StdSerial Serial();

StdSerial::StdSerial(void)
{
}

void StdSerial::begin(int speed)
{
}

void StdSerial::print(char c)
{
	putchar(str);
}

void StdSerial::print(char *str)
{
	puts(str);
}

void StdSerial::println(char *str)
{
	printf("%s\n", str);
}

void StdSerial::println(void)
{
	putchar('\n');
}

void StdSerial::println(unsigned int i)
{
	printf("%d\n", i);
}

int  StdSerial::available(void)
{
	XXX
}

int  StdSerial::read(void)
{
	return getchar();
}


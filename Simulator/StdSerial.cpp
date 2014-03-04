#include "StdSerial.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

StdSerial Serial = StdSerial();

StdSerial::StdSerial()
{
}

void StdSerial::begin(int speed)
{
}

void StdSerial::print(char c)
{
	putchar(c);
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
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(fileno(stdin), &fds);
	select(fileno(stdin)+1, &fds, NULL, NULL, &tv);
	return (FD_ISSET(0, &fds));
}

int  StdSerial::read(void)
{
	return getchar();
}


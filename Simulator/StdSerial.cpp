#include <gtk/gtk.h>
#include "StdSerial.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

StdSerial Serial = StdSerial();

StdSerial::StdSerial()
{
}

void StdSerial::set_fd(int fd)
{
	if((this->f = fdopen(fd, "rw")) == NULL) {
		g_print("Cannot open fd %d.\n", fd);
		exit(1);
	}
}

void StdSerial::begin(int speed)
{
}

void StdSerial::print(char c)
{
	fputc(c, this->f);
}

void StdSerial::print(const char *str)
{
	fputs(str, this->f);
}

void StdSerial::println(const char *str)
{
	fprintf(this->f, "%s\n", str);
}

void StdSerial::println(void)
{
	fputc('\n', this->f);
}

void StdSerial::println(unsigned int i)
{
	fprintf(this->f, "%d\n", i);
}

int  StdSerial::available(void)
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(fileno(this->f), &fds);
	select(fileno(this->f)+1, &fds, NULL, NULL, &tv);
	return (FD_ISSET(0, &fds));
}

int  StdSerial::read(void)
{
	return fgetc(this->f);
}


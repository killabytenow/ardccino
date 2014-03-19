#include <gtk/gtk.h>
#include "StdSerial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

StdSerial Serial = StdSerial();

StdSerial::StdSerial()
{
}

void StdSerial::set_fd(int fd)
{
	this->fd = fd;
	g_print(__FILE__ ":%s: writing to fd %d\n", __func__, this->fd);
}

void StdSerial::begin(int speed)
{
}

void StdSerial::print(char c)
{
	write(this->fd, &c, sizeof(char));
}

void StdSerial::print(const char *str)
{
	write(this->fd, str, strlen(str));
}

void StdSerial::println(const char *str)
{
	this->print(str);
	this->println();
}

void StdSerial::println(void)
{
	char nl = '\n';
	write(this->fd, &nl, sizeof(char));
}

void StdSerial::println(unsigned int i)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer) - 1, "%d\n", i);
	this->println(buffer);
}

int  StdSerial::available(void)
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(this->fd, &fds);
	select(this->fd+1, &fds, NULL, NULL, &tv);
	return (FD_ISSET(0, &fds));
}

int  StdSerial::read(void)
{
	char c;
	::read(this->fd, (void *) &c, 1);
	return c;
}


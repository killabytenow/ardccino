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
}

void StdSerial::begin(int speed)
{
}

void StdSerial::print(char c)
{
	write(this->fd, &c, sizeof(char));
}

void StdSerial::print(int i)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer) - 1, "%d", i);
	buffer[1023] = '\0';
	write(this->fd, buffer, strlen(buffer));
}

void StdSerial::print(unsigned int i)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer) - 1, "%u", i);
	buffer[1023] = '\0';
	write(this->fd, buffer, strlen(buffer));
}

void StdSerial::print(const char *str)
{
	write(this->fd, str, strlen(str));
}

void StdSerial::println(char c)          { this->print(c);    this->println(); }
void StdSerial::println(int i)           { this->print(i);    this->println(); }
void StdSerial::println(unsigned int i)  { this->print(i);    this->println(); }
void StdSerial::println(const char *str) { this->print(str);  this->println(); }
void StdSerial::println(void)            { this->print('\n');                  }

int StdSerial::available(void)
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(this->fd, &fds);
	select(this->fd+1, &fds, NULL, NULL, &tv);
	return (FD_ISSET(this->fd, &fds));
}

int  StdSerial::read(void)
{
	char c;
	::read(this->fd, (void *) &c, 1);
	return c;
}


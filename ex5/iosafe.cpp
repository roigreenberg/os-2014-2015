/*
 * iosafe.cpp
 *
 *  Created on: 12 jun 2015
 *      Author: roigreenberg
 */

#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include "iosafe.h"

using namespace std;


/*
 * this function read or write until it finish all the requested bytes
 */
int io_safe(ssize_t (*f)(int, void*, size_t), int src, char *buf, unsigned int bufSize)
{
	unsigned int count =0;
	int tmp = 0;

	while (count < bufSize)
	{
		if ((tmp = f(src, buf + count, bufSize - count)) > 0)
		{
			count += tmp;
		}
		else if (tmp < 0)
		{
			return -1;
		}

	}
	return count;
}


/*
 * helper for read
 */
ssize_t safe_read(int src, void* buf, size_t size)
{
	return read(src, buf, size);
}

/*
 * helper for write
 */
ssize_t safe_write(int dst, void* buf, size_t size)
{
	return write(dst, buf, size);
}

/*
 * send ALL the data from dst to src
 */
int safe_send(int src, int dst, char* buf, unsigned int bufSize, size_t size)
{

	int rcount = 0;
	int wcount = 0;
	unsigned int count = 0;
	unsigned int left = size;

	while (count < size)
	{
		if (left < bufSize)
		{
			bufSize = left;
		}
		rcount = io_safe(&safe_read, src, buf, bufSize);//errors?
		if (rcount == -1)
		{
			throw SystemError("read");
		}
		wcount = io_safe(&safe_write, dst, buf, rcount);

		if (wcount == -1)
		{
			throw SystemError("write");
		}
		count += wcount;
		left -= wcount	;
	}

	return count;
}

/*
 * sent int to dst
 */
int send_size(int dst, char  *buf , unsigned int size)
{
	int n;
	unsigned char * p_size = (unsigned char * )&size;
	memcpy(buf, p_size, 4);
	if ((n = safe_write(dst, buf, 4)) != 4)
	{
		throw SystemError("write");
	}
	return n;
}

/*
 * sent name to dst
 */
int send_name(int dst, char  *buf , char * name, unsigned int size)
{
	unsigned int n;
	memcpy(buf, name, size);
	if ((n = safe_write(dst, buf, size)) != size)
	{
		throw SystemError("write");
	}
	for (size_t i = 0; i < size; i++)
	{
		buf[i] = '\0';
	}
	return n;
}


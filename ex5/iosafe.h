/*
 * iosafe.h
 *
 *  Created on: 12 jun 2015
 *      Author: roigreenberg
 */

#ifndef IOSAFE_H_
#define IOSAFE_H_

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

using namespace std;

class SystemError: public exception
{
	char const* error_message;
public:
	SystemError(char const* error_name)
	{
		error_message = error_name;
	}
	virtual const char* what() const throw()
	{
		return error_message;
	}
};

/*
 * this function read or write until it finish all the requested bytes
 */
int io_safe(ssize_t (*f)(int, void*, size_t), int src, char *buf, unsigned int bufSize);

/*
 * helper for read
 */
ssize_t safe_read(int src, void* buf, size_t size);

/*
 * helper for write
 */
ssize_t safe_write(int dst, void* buf, size_t size);

/*
 * send ALL the data from dst to src
 */
int safe_send(int src, int dst, char* buf, unsigned int bufSize, size_t size);

/*
 * sent int to dst
 */
int send_size(int dst, char  *buf , unsigned int size);

/*
 * sent name to dst
 */
int send_name(int dst, char  *buf , char * name, unsigned int size);




#endif /* IOSAFE_H_ */

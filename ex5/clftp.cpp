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

#define USAGE "Usage: clftp server-port server-hostname file-to-transfer filename-in-server"
#define BUF_SIZE 100000

using namespace std;

void inline sys_error_handle(const char* system_call)
{
	cerr << "Error: function:" << system_call << " errno:" << errno << ".\n";
	exit(1);
}

/*
 * call the socket
 */
int call_socket(char *hostname, unsigned short port_num)
{
	int s;
	struct sockaddr_in sa;
	struct hostent *hp;
	memset(&sa, 0, sizeof(sa));

	if ((hp = gethostbyname(hostname)) == NULL)
	{
		errno= h_errno;
		sys_error_handle("gethostbyname");
	}

	sa.sin_family = hp->h_addrtype;
	memcpy((char*)&sa.sin_addr, hp->h_addr, hp->h_length);
	sa.sin_port = htons((u_short)port_num);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		sys_error_handle("socket");
	}

	if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
	{
		sys_error_handle("connect");
	}

	return s;
}

/*
 * get the max file size from server and check if the file size is valid
 */
bool check_size(int src, char* buf, struct stat &statbuf)
{
	try
	{
		safe_read(src, buf, 4);
	}
	catch(exception& e)
	{
		sys_error_handle(e.what());
	}
	unsigned int maxSize = *(unsigned int*)buf;


	if (maxSize < statbuf.st_size)
	{
		return false;
	}
	return true;
}

int main(int argc, char* argv[])
{
	if (argc != 5)
	{
		cout << USAGE << endl;
		exit(1);
	}

	int s_port = atoi(argv[1]);
	char * s_hostname = argv[2];
	char * file_to_transfer = argv[3];
	char * file_in_server = argv[4];
	
	if (s_port < 1 || s_port > 65535 || (ifstream(file_to_transfer) == 0))
	{
		cout << USAGE << endl;
		exit(1);
	}

	//call the socket
	int s;
	s = call_socket(s_hostname, s_port);

	//create buffer
	char * buf = new char[BUF_SIZE];

	//to know and check file size
	struct stat statbuf;
	if (lstat(file_to_transfer, &statbuf) != 0)
	{
		sys_error_handle("lstat");
	}
	if(S_ISDIR(statbuf.st_mode))
	{
		cout << USAGE << endl;
		exit(1);
	}

	if (check_size(s, buf, statbuf) == false)
	{
		cout << "Transmission failed: too big file" << endl;
		send_size(s, buf, -1);
		delete[] buf;
		return 0;
	}

	unsigned int size = statbuf.st_size;

	int f = open(file_to_transfer, 'R');
	if (f < 0)
	{
		sys_error_handle("open");
	}
	try
	{
		send_size(s, buf, size); // sent file size
		send_size(s, buf, string(file_in_server).size()); // send file in server length
		send_name(s, buf, file_in_server, string(file_in_server).size()); // senr file in server name
		safe_send(f, s, buf, BUF_SIZE, size);
	}
	catch(exception& e)
	{
		sys_error_handle(e.what());
	}
	delete[] buf;
	if (close(f)==-1)
	{
		sys_error_handle("close");
	}
	return 0;
}
;

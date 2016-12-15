#include <iostream>
#include <fstream>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>
#include "iosafe.h"

#define BUF_SIZE 10000
#define USAGE "Usage: srftp server-port max-file-size"

using namespace std;

int s_port;
int max_file_size;

void inline sys_error_handle(const char* system_call)
{
	cerr<<"Error: function:" << system_call << " errno:" << errno << ".\n";
	pthread_exit(NULL);
}

int establish(unsigned short port_num)
{
	char myname[HOST_NAME_MAX + 1];
	int s;
	struct sockaddr_in sa;
	struct hostent *hp;
	memset(&sa, 0, sizeof(struct sockaddr_in));


	if(gethostname(myname, HOST_NAME_MAX) == -1)
	{
		sys_error_handle("gethostname");
	}
	hp = gethostbyname(myname);
	if (hp == NULL)
	{
		errno= h_errno;
		sys_error_handle("gethostbyname");
	}
	sa.sin_family = hp->h_addrtype;

	sa.sin_port = htons((int)port_num);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		sys_error_handle("socket");
	}
	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0)
	{
		sys_error_handle("bind");
	}
	if (listen(s, 5) < 0)
	{
		sys_error_handle("listen");
	}
	return (s);
}

int get_connection(int s)
{
	int ns;
	if ((ns = accept(s, NULL, NULL)) < 0)
	{
		sys_error_handle("accept");
	}
	return ns;
}


int client(int ns)
{
	char* buf = new char[BUF_SIZE];
	try
	{
		send_size(ns, buf, max_file_size);
		int i;
		// read file size
		if ((i = safe_read(ns, buf, 4)) < 0)
		{
			sys_error_handle("read");
		}
		if (*(int*)buf == -1)
		{
			delete[] buf;
			close(ns);
			return 0;
		}
		unsigned int size = *(unsigned int*)buf;

		// read file name length
		if ((i = safe_read(ns, buf, 4)) < 0)
		{
			sys_error_handle("read");
		}

		unsigned int nameSize = *(unsigned int*)buf;

		//read file name
		char * filename = new char[nameSize+1];
		if ((i = safe_read(ns, filename, nameSize)) < 0)
		{
			sys_error_handle("read");
		}
		filename[nameSize] = '\0'; //make sure file name end properly

		int f;

		ofstream of (filename);
		of.close();

		if ((f = open (filename, O_WRONLY)) < 0)
		{
			delete[] filename;
			delete[] buf;
			close(ns);
			sys_error_handle("open");
		}
		//write data to file
		safe_send(ns, f, buf, BUF_SIZE, size);
		delete[] filename;
		delete[] buf;
		if (close(f)==-1 || close(ns)==-1)
		{
			sys_error_handle("close");
		}
	}
	catch (exception& e)
	{
		sys_error_handle(e.what());
	}
	return 0;
}
/*
 * helper function for the client thread
 * convert the ns to int
 */
void* client_thread(void* ns)
{
	int s = *(int*)ns;
	delete (int*)ns;
	client(s);
	return NULL;
}

int main(int argc, char* argv[])
{
	int ns;

	if (argc != 3)
	{
		cout << USAGE << endl;
		exit(1);
	}

	s_port = atoi(argv[1]);
	const char * max_file = argv[2];
	max_file_size = atoi(max_file);

	if (s_port < 1 || s_port > 65535 || max_file_size < 0)
	{
		cout << USAGE << endl;
		exit(1);
	}

	int s = establish(s_port);
	while (true)
	{
		ns = get_connection(s);
		pthread_t client;
		if (pthread_create(&client, NULL, client_thread, (void*)new int(ns))!=0)
		{
			sys_error_handle("pthread_create");
		}
		pthread_detach(client);
	}
	return s;
}

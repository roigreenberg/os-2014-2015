CC = g++ -Wall
FLAG = -std=c++11
LIBSRC = srftp.cpp clftp.cpp iosafe.cpp iosafe.h

all: srftp clftp 




srftp.o: iosafe.h
	$(CC) $(FLAG) srftp.cpp -c

clftp.o: iosafe.h
	$(CC) $(FLAG) clftp.cpp -c


srftp: srftp.o
	$(CC) $(FLAG)  srftp.o iosafe.cpp -lpthread -L./ -o srftp

clftp: clftp.o
	$(CC) $(FLAG) clftp.o iosafe.cpp -o clftp

TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS=$(LIBSRC) Makefile README performance.jpg

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

clean:
	$(RM) *.o srftp clftp  *~ *core


.PHONY: clftp clftp.o srftp srftp.o iosafe.o

CC = g++ -Wall
FLAG = -std=c++11 -D_FILE_OFFSET_BITS=64
FUSE = `pkg-config fuse --cflags --libs`
LIBSRC = CachingFileSystem.cpp

all: CachingFileSystem

CachingFileSystem.o: CachingFileSystem.cpp
	g++ -std=c++11 -Wall -c -fmessage-length=0 -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags` CachingFileSystem.cpp

CachingFileSystem: CachingFileSystem.o
	g++ -Wall -std=c++11 -D_FILE_OFFSET_BITS=64 CachingFileSystem.o `pkg-config fuse --libs` -o CachingFileSystem

TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
TARSRCS=$(LIBSRC) Makefile README

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)

clean:
	$(RM) CachingFileSystem.o CachingFileSystem *~ *core


.PHONY: CachingFileSystem CachingFileSystem.o

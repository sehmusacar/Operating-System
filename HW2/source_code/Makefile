CC = gcc
CFLAGS = -Wall -g

all: makeFileSystem fileSystemOper

makeFileSystem: makeFileSystem.c
	$(CC) $(CFLAGS) -o makeFileSystem makeFileSystem.c

fileSystemOper: fileSystemOper.c
	$(CC) $(CFLAGS) -o fileSystemOper fileSystemOper.c

clean:
	rm -f makeFileSystem fileSystemOper mySystem.dat linuxFile.data linuxFile2.data

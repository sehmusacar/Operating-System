#!/bin/bash

# Creates a file system named fileSystem.dat with 1 KB block size
./makeFileSystem 1 fileSystem.dat

# Creates a test file (test1.txt) with 100 KB of content "gsgsgsgs"
yes "gsgsgsgs" | head -c 100K > test1.txt

# Creates a test file (test2.txt) with 75 KB of content "deneme"
yes "deneme" | head -c 75K > test2.txt

# Creates the /usr directory
./fileSystemOper fileSystem.dat mkdir "/usr"

# Creates the /usr/ysa directory
./fileSystemOper fileSystem.dat mkdir "/usr/ysa"

# Writes the file test1.txt to /usr/ysa/test1 in the fileSystem.dat file system
./fileSystemOper fileSystem.dat write "/usr/ysa/test1" test1.txt

# Writes the file test2.txt to /usr/ysa/test2 in the fileSystem.dat file system
./fileSystemOper fileSystem.dat write "/usr/ysa/test2" test2.txt

# Lists the contents of the /usr/ysa directory
./fileSystemOper fileSystem.dat dir "/usr/ysa"

# Displays file system information and contents
./fileSystemOper fileSystem.dat dumpe2fs

# Deletes the file /usr/ysa/test2
./fileSystemOper fileSystem.dat del "/usr/ysa/test2"

# Attempts to remove the /usr/ysa directory (will fail if it contains files)
./fileSystemOper fileSystem.dat rmdir "/usr/ysa"

# Displays file system information and contents
./fileSystemOper fileSystem.dat dumpe2fs

# Deletes the file /usr/ysa/test1
./fileSystemOper fileSystem.dat del "/usr/ysa/test1"

# Removes the /usr directory
./fileSystemOper fileSystem.dat rmdir "/usr"

# Displays file system information and contents
./fileSystemOper fileSystem.dat dumpe2fs

# Writes the file test2.txt to /usr/ysa/test2 in the fileSystem.dat file system again
./fileSystemOper fileSystem.dat write "/usr/ysa/test2" test2.txt

# Changes the permissions of /usr/ysa/test2 to -rw
./fileSystemOper fileSystem.dat chmod "/usr/ysa/test2" -rw

# Reads the file /usr/ysa/test1 and writes its content to test2.txt
./fileSystemOper fileSystem.dat read "/usr/ysa/test1" test2.txt

# Displays file system information and contents
./fileSystemOper fileSystem.dat dumpe2fs

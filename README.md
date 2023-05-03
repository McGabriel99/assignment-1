# assignment-1 - FTP Client-Server
##CPSC 471 Computer Commnunications - Programming Assignment

### Problem description:
Implement a simplified FTP server and FTP client. The client connects to the server and support uploading and downloading of files to/from the server using TCP/IP.

### Goals:
1. To understand the challenges of protocol design
2. To discover and appreciate the challenges of developing complex, real-world network applications
3. Make sense of real-world sockets programming APIs
4. To utilize a sockets programming API to construct simplified FTP server and client applications

***

### Group members:
```
Mc Gabriel Fernandez: mcgabrielf@csu.fullerton.edu
Nathan Mayne: nmayne3@gmail.com
Harry Dinh: 20hdinh@csu.fullerton.edu
Alex Chavez: schavez95@csu.fullerton.edu
```
***
### Programming Language Used: ```C/C++```

***
### How to Execute:

#### Step 1:
```
$ git clone <repo>
$ cd ../path/to/the/file
```
#### Step 2:
```
$ make clean --> to delete executables
$ make
```
#### Step 3:
1. Open new terminal for **CLIENT**
```
$ cd cli/
```
2. Open new terminal for **SERVER**
```
$ cd ser/
```
#### Step 4:
1. In Server terminal
```
$ ./server <port number>
```
2. In Client terminal
```
./client <IP addr> <server's port number>
```

***
### How the program works:
Once the FTP server and client programs are running, the **client** program can run these following commands:
```
$ get <filename> - client downloads <filename> from the server
$ put <filename> - client uploads <filename> to the server
$ ls - client receives file list from the server's directory
$ quit - terminates session between client and server
```


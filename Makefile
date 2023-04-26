all: client server

client:	cli/ftpclient.cpp TCPLib.o
	g++ cli/ftpclient.cpp TCPLib.o -o cli/client

server: ser/ftpserver.cpp TCPLib.o
	g++ ser/ftpserver.cpp TCPLib.o -o ser/server

TCPLib.o:	TCPLib.h TCPLib.cpp
	g++ -c TCPLib.cpp 

clean:
	rm -rf ser/server cli/client TCPLib.o 

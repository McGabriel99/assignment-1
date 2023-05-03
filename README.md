# assignment-1 - FTP Client-Server
##CPSC 471 Computer Commnunications - Programming Assignment

## Table of Contents
1. [Problem description](#problem-description)

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



## Table of Contents
1. [General Info](#general-info)
2. [Technologies](#technologies)
3. [Installation](#installation)
4. [Collaboration](#collaboration)
5. [FAQs](#faqs)
### General Info
***
Write down general information about your project. It is a good idea to always put a project status in the readme file. This is where you can add it. 
### Screenshot
![Image text](https://www.united-internet.de/fileadmin/user_upload/Brands/Downloads/Logo_IONOS_by.jpg)
## Technologies
***
A list of technologies used within the project:
* [Technology name](https://example.com): Version 12.3 
* [Technology name](https://example.com): Version 2.34
* [Library name](https://example.com): Version 1234
## Installation
***
A little intro about the installation. 
```
$ git clone https://example.com
$ cd ../path/to/the/file
$ npm install
$ npm start
```
Side information: To use the application in a special environment use ```lorem ipsum``` to start
## Collaboration
***
Give instructions on how to collaborate with your project.
> Maybe you want to write a quote in this part. 
> Should it encompass several lines?
> This is how you do it.
## FAQs
***
A list of frequently asked questions
1. **This is a question in bold**
Answer to the first question with _italic words_. 
2. __Second question in bold__ 
To answer this question, we use an unordered list:
* First point
* Second Point
* Third point
3. **Third question in bold**
Answer to the third question with *italic words*.
4. **Fourth question in bold**
| Headline 1 in the tablehead | Headline 2 in the tablehead | Headline 3 in the tablehead |
|:--------------|:-------------:|--------------:|
| text-align left | text-align center | text-align right |

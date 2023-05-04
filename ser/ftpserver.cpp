#include <iostream>        //for cout, cerr
#include <fstream>         //for ifstream, ofstream
#include <ctime>           //for time
#include <sys/stat.h>      //for stat to get file size
#include <dirent.h>        //for DIR, opendir, readdir, closedir
#include "../TCPLib.h"     //for tcp_send, tcp_recv

#define BUFFER_SIZE 1024

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::ios;
using std::stoi;
using std::rand;
using std::srand;
using std::min;

int create_server_socket(int port);
int create_data_socket(int port);
int send_data_port(int control_socket, int data_port);
int accept_client_connection(int server_socket);
int accept_data_connection(int server_socket);
void handle_client_commands(int control_socket);
int receive_command(int control_socket, string &out_command);
int handle_get_command(int control_socket, const char* filename);
int handle_put_command(int control_socket, const char* filename);
int handle_ls_command(int control_socket);
int send_file_list(int data_socket);
int send_file(int data_socket, const char* filename, int file_size);
int send_file_size(int control_socket, const char* filename);
int receive_file(int data_socket, const char* filename, int file_size);
int receive_file_size(int control_socket);
void display_progress_bar(long long current, long long total);


int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc != 2) {
        //./server <port>
        cout << "Usage: " << argv[0] << " <server_port>" << endl;
        return 1;
    }
    //convert port to int, and create server socket
    int port = stoi(argv[1]);
    int control_socket = create_server_socket(port);

    if (control_socket < 0) {
        cerr << "Failed to create control socket" << endl;
        return 1;
    }

    cout << "Server listening on port " << port << endl;

    //accept client connection
    while (true) {
        int client_control_socket = accept_client_connection(control_socket);
        if (client_control_socket < 0) {
            cerr << "Failed to accept control connection" << endl;
            continue;
        }

        //When a client connects, enter a loop to handle its commands
        while (true) {
            cout << "-Waiting for client commands-\n";
            string command;
            if (receive_command(client_control_socket, command) < 0) {
                cout << "Failed to receive command" << endl;
                break;
            }

            if (command == "quit") {
                cout << "Client requested to quit" << endl;
                close(client_control_socket);
                break;
            }
            else if (command.substr(0, 3) == "get") {
                string filename = command.substr(4);
                if (handle_get_command(client_control_socket, filename.c_str()) < 0) {
                    cerr << "Failed to handle GET command" << endl;
                }
            } 
            else if (command.substr(0, 3) == "put") {
                string filename = command.substr(4);
                if (handle_put_command(client_control_socket, filename.c_str()) < 0) {
                    cerr << "Failed to handle PUT command" << endl;
                }
            } else if (command == "ls") {
                if (handle_ls_command(client_control_socket) < 0) {
                    cerr << "Failed to handle LS command" << endl;
                }
            } else {
                cerr << "Invalid command received: " << command << endl;
            }
        }

        close(client_control_socket);
    }

    close(control_socket);
    return 0;
}




/*
    * Creates a server socket and listens on the specified port for client connection
    @param port - the port to listen on
    @return - the server socket, or -1 on error
*/
int create_server_socket(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return -1;
    }
    //set socket options to reuse address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    //bind socket to address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return -1;
    }
    //listen for connections
    if (listen(server_socket, 100) < 0) {
        perror("listen");
        close(server_socket);
        return -1;
    }

    return server_socket;
}


/*
    * Creates a data socket and listens on the specified port for data file transfer
    @param port - the port to listen on
    @return - the data socket, or -1 on error
*/
int create_data_socket(int port) {
    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("socket");
        return -1;
    }
    //set socket options to reuse address
    struct sockaddr_in data_addr;
    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY;
    data_addr.sin_port = htons(port);
    //bind socket to address
    if (bind(data_socket, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        perror("bind");
        close(data_socket);
        return -1;
    }
    //listen for connections
    if (listen(data_socket, 100) < 0) {
        perror("listen");
        close(data_socket);
        return -1;
    }

    return data_socket;
}

/*
    * Sends the data port to the client
    @param control_socket - the control socket to send the data port to
    @param data_port - the data port to send
    @return - 0 on success, -1 on error
*/
int send_data_port(int control_socket, int data_port) {
    char data_port_str[32];
    sprintf(data_port_str, "%d", data_port);
    if (tcp_send(control_socket, data_port_str, sizeof(data_port_str) - 1) < 0) {
        perror("tcp_send");
        return -1;
    }
    return 0;
}

/*
    * Accepts a client connection on the specified server socket
    @param server_socket - the server socket to accept the connection on
    @return - the client socket, or -1 on error
*/
int accept_client_connection(int server_socket) {
    // create sockaddr_in to store client address info
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    // Accept incoming connection
    int control_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (control_socket < 0) {
        perror("accept");
        return -1;
    }
    //print client info
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    cout << "Client connected from " << client_ip << ":" << ntohs(client_addr.sin_port) << endl;

    return control_socket;
}

/*
    * Accepts a data connection on the specified server socket for file transfer
    @param server_socket - the server socket to accept the connection on
    @return - the data socket, or -1 on error
*/
int accept_data_connection(int server_socket) {
    // create sockaddr_in to store client address info
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Accept incoming connection
    int data_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (data_socket < 0) {
        perror("accept");
        return -1;
    }
    return data_socket;
}

/*
    * Handles client commands
    @param control_socket - the control socket to receive commands from
    @param out_command - the command received from the client
    @return - 0 on success, -1 on error
*/
int receive_command(int control_socket, string &out_command) {
    //buffer to store command
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));


    int bytes_received = recv(control_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        return -1;
    }
    //null terminate the command
    buffer[bytes_received] = '\0';
    out_command = string(buffer);
    return 0;
}

/*
    * Handles the GET command
    @param control_socket - the control socket to send data to
    @param filename - the name of the file to send
    @return - 0 on success, -1 on error
*/
int handle_get_command(int control_socket, const char* filename) {
    cout << "\n==========Handling GET command==========" << endl;

    //file size is sent first
    int file_size = send_file_size(control_socket, filename);
    if (file_size < 0) {
        cerr << "Failed to send file size" << endl;
        return -1;
    }
    cerr << "File size: " << file_size << endl;
    //data port is sent next
    // generate random port between 1024 and 65535
    int data_port = rand() % (65535 - 1024 + 1) + 1024;
    if (send_data_port(control_socket, data_port) < 0) {
        cerr << "Failed to send data port" << endl;
        return -1;
    }

    //create data socket for file transfer
    cerr << "Data port: " << data_port << endl;
    int data_socket = create_data_socket(data_port);
    if (data_socket < 0) {
        cerr << "Failed to create data socket" << endl;
        return -1;
    }
    //accept data connection from client for file transfer
    cout << "Accepting connection" << endl;
    int client_data_socket = accept_data_connection(data_socket);
    if (client_data_socket < 0) {
        cerr << "Failed to accept data connection" << endl;
        close(data_socket);
        return -1;
    }
    cout << "Connection accepted" << endl;
    cout << "Sending file: " << filename << endl;
    //send file and file size
    if (send_file(client_data_socket, filename, file_size) < 0) {
        cerr << "Failed to send file" << endl;
        close(client_data_socket);
        close(data_socket);
        return -1;
    }
    cout << "File sent" << endl;
    cout << "==========Exiting GET command==========" << endl;

    //close sockets
    close(client_data_socket);
    close(data_socket);
    return 0;
}

/*
    * Handles the PUT command
    @param control_socket - the control socket to receive data from
    @param filename - the name of the file to receive
    @return - 0 on success, -1 on error
*/

int handle_put_command(int control_socket, const char* filename) {
    cout << "\n==========Handling PUT command==========" << endl;

    // Receive file size
    int file_size = receive_file_size(control_socket);
    if (file_size < 0) {
        cerr << "Failed to receive file size" << endl;
        return -1;
    }
    // generate random data port to use
    int data_port = rand() % (65535 - 1024 + 1) + 1024;
    // send data port to client to connect to
    if (send_data_port(control_socket, data_port) < 0) {
        cerr << "Failed to send data port" << endl;
        return -1;
    }
    // Create data socket for file transfer
    int data_socket = create_data_socket(data_port);
    if (data_socket < 0) {
        cerr << "Failed to create data socket" << endl;
        return -1;
    }
    // Accept data connection from client for file transfer
    int client_data_socket = accept_data_connection(data_socket);
    if (client_data_socket < 0) {
        cerr << "Failed to accept data connection" << endl;
        close(data_socket);
        return -1;
    }

    // Close the data_socket as it's no longer needed
    close(data_socket);

    // Receive file and file size
    cout << "Receiving file: " << filename << endl;
    if (receive_file(client_data_socket, filename, file_size) < 0) {
        cerr << "Failed to receive file" << endl;
        close(client_data_socket);
        return -1;
    }

    cout << "==========Exiting PUT command==========" << endl;

    close(client_data_socket);
    return 0;
}

/*
    * Handles the LS command
    @param control_socket - the control socket to send data to
    @return - 0 on success, -1 on error
*/
int handle_ls_command(int control_socket) {
    cout << "\n==========Handling LS command==========" << endl;
    // Generate random port number between 1024 and 65535
    int data_port = rand() % (65535 - 1024 + 1) + 1024;
    //cout << "Data port: " << data_port << endl;
    // Send port number over control connection
    if (send_data_port(control_socket, data_port) < 0) {
        cerr << "Failed to send data port" << endl;
        return -1;
    }
    // Create a socket for data transfer
    int data_socket = create_data_socket(data_port);
    if (data_socket < 0) {
        cerr << "Failed to create data socket" << endl;
        return -1;
    }
    //cout << "Data socket: " << data_socket << endl; 
    // Accept connection from client
    int client_data_socket = accept_data_connection(data_socket);
    if (client_data_socket < 0) {
        cerr << "Failed to accept data connection" << endl;
        close(data_socket);
        return -1;
    }

    // Close the data_socket as it's no longer needed
    close(data_socket);

    // Send the list of directory to client
    if (send_file_list(client_data_socket) < 0) {
        cerr << "Failed to send file list" << endl;
        close(client_data_socket);
        return -1;
    }

    cout << "==========Exiting LS command==========" << endl;
    //close the data socket
    close(client_data_socket);
    return 0;
}

/*
    * Handles the EXIT command
    @param control_socket - the control socket to send data to
    @return - 0 on success, -1 on error
*/

int send_file_list(int data_socket) {
    cout << "Directed to send file list" << endl;
    // Open the current directory
    DIR *dir;
    struct dirent *entry;
    string file_list;
    if (!(dir = opendir("."))) {
        perror("opendir");
        return -1;
    }
    cout << "Reading Directory" << endl;
    // Read the directory entries
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            cout << "File: " << entry->d_name << endl;
            // Append the file name to the file list
            file_list.append(entry->d_name);
            file_list.append("\n");
        }
    }
    cout << "Directory Read... Sending File List" << endl;
    // Close the directory
    closedir(dir);
    cout << "Directory Closed" << endl;

    // Send the file list to client
    if (tcp_send(data_socket, file_list.c_str(), file_list.size()) < 0) {
        perror("tcp_send");
        return -1;
    }

    return 0;
}


/*
    * sends file to client through data socket
    @param data_socket - the data socket to send data to
    @param filename - the name of the file to send
    @param file_size - the size of the file to send
*/
int send_file(int data_socket, const char* filename, int file_size) {
    // Open the file for reading
    FILE* file = fopen(filename, "rb");
    if (file == nullptr) {
        perror("fopen");
        return -1;
    }

    // Send the file content in chunks
    char buffer[BUFFER_SIZE];
    long long bytes_sent = 0;

    while (bytes_sent < file_size) {
        size_t bytes_to_send = min(static_cast<long long>(BUFFER_SIZE), file_size - bytes_sent);
        size_t bytes_read = fread(buffer, 1, bytes_to_send, file);

        // Check for fread error
        if (bytes_read < 1) {
            perror("fread");
            fclose(file);
            return -1;
        }
        // Send the chunk
        if (tcp_send(data_socket, buffer, bytes_read) < 0) {
            perror("tcp_send");
            fclose(file);
            return -1;
        }

        bytes_sent += bytes_read;
        
        display_progress_bar(bytes_sent, file_size);
    }

    // Close the file
    fclose(file);
    return 0;
}

/*
    * sends file size to client through control socket
    * the redundancy of this function is to make sure that the client receives the file size
      before the data socket is created, and also if the file size is needed for other purposes
    @param control_socket - the control socket to send data to
    @param filename - the name of the file to send
    @return - the size of the file to send
*/
int send_file_size(int control_socket, const char* filename) {
    // stat the file to get the file size
    struct stat file_stat;
    if (stat(filename, &file_stat) < 0) {
        perror("stat");
        return -1;
    }
    // convert file size to string
    int file_size = file_stat.st_size;
    char file_size_str[32];
    sprintf(file_size_str, "%d", file_size);
    // send file size to client
    if (tcp_send(control_socket, file_size_str, sizeof(file_size_str)-1 ) < 0) {
        perror("tcp_send");
        return -1;
    }
    //cerr << "Filesize sent" << endl;
    return file_size;
}

/*
    * receives file from client through data socket
    @param data_socket - the data socket to receive data from
    @param filename - the name of the file to receive
    @param file_size - the size of the file to receive
    @return - 0 on success, -1 on error
*/
int receive_file(int data_socket, const char* filename, int file_size) {
    // open the file for writing
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "Error: Unable to create the file." << endl;
        return 1;
    }
    // receive the file content in chunks
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int total_bytes_received = 0;

    while (total_bytes_received < file_size) {
        // receive the chunk
        bytes_received = tcp_recv(data_socket, buffer, sizeof(buffer));
        file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;
        display_progress_bar(total_bytes_received, file_size);
    }

    cout << "\nFile transfer completed." << endl;
    file.close();
    return 0;
}

/*
    * receives file size from client through control socket
    * the redundancy of this function is to make sure that the client sends the file size
      before the data socket is created, and also if the file size is needed for other purposes
    @param control_socket - the control socket to receive data from
    @return - the size of the file to receive
*/
int receive_file_size(int control_socket) {
    // receive file size from client
    char file_size_str[32];
    tcp_recv(control_socket, file_size_str, sizeof(file_size_str)-1);
    int  file_size = stoi(file_size_str);
    return file_size;
}


/*
    * displays the progress bar for file transfer
    @param current - the current number of bytes transferred
    @param total - the total number of bytes to transfer
*/
void display_progress_bar(long long current, long long total) {
    const int bar_width = 50;
    float progress = static_cast<float>(current) / total;
    int position = static_cast<int>(bar_width * progress);
    cout << "Transferring file:";
    cout << "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < position) {
            cout << "#";
        } else if (i == position) {
            cout << ">";
        } else {
            cout << " ";
        }
    }

    cout << "] " << static_cast<int>(progress * 100.0) << " %\r";
    cout.flush();

    cout << endl;
}
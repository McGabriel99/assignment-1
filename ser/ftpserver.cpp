#include <iostream>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <dirent.h>
#include "../TCPLib.h"

#define BUFFER_SIZE 1024

using namespace std;

int create_server_socket(int port);
int create_data_socket(int port);
int send_data_port(int control_socket, int data_port);
int accept_client_connection(int server_socket);
int accept_data_connection(int server_socket);
void handle_client_commands(int control_socket);
string receive_command(int control_socket);
int handle_get_command(int control_socket, const char* filename);
int handle_put_command(int control_socket, const char* filename);
int handle_ls_command(int control_socket);
int send_file_list(int data_socket);
int send_file(int data_socket, const char* filename, int file_size);
int send_file_size(int control_socket, const char* filename);
int receive_file(int data_socket, const char* filename, int file_size);
int receive_file_size(int control_socket);
void display_progress_bar(long long current, long long total);


// int main(int argc, char *argv[]) {
//     srand(time(NULL));
//     if (argc != 2) {
//         cerr << "Usage: " << argv[0] << " <port>" << endl;
//         exit(EXIT_FAILURE);
//     }

//     int port = atoi(argv[1]);
//     int server_socket = create_server_socket(port);
//     if (server_socket < 0) {
//         cerr << "Failed to create server socket" << endl;
//         exit(EXIT_FAILURE);
//     }

//     cout << "Server listening on port " << port << endl;

//     while (true) {
//         int control_socket = accept_client_connection(server_socket);
//         if (control_socket < 0) {
//             cerr << "Failed to accept client connection" << endl;
//             continue;
//         }


//         if (fork() == 0) { // In the child process
//             close(server_socket);
//             handle_client_commands(control_socket);
//             cout << "Client disconnected" << endl;
//             close(control_socket);
//             exit(EXIT_SUCCESS);
//         } else { // In the parent process
//             close(control_socket);
//         }
//     }

//     close(server_socket);
//     return 0;
// }

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <server_port>" << endl;
        return 1;
    }

    int port = stoi(argv[1]);
    int control_socket = create_server_socket(port);
    //cout << "server-side control_socket: " << control_socket << endl;
    if (control_socket < 0) {
        cerr << "Failed to create control socket" << endl;
        return 1;
    }

    while (true) {
        int client_control_socket = accept_client_connection(control_socket);
        //cout << "Client-Control socket: " << client_control_socket << endl;
        if (client_control_socket < 0) {
            cerr << "Failed to accept control connection" << endl;
            continue;
        }

        while (true) {
            string command = receive_command(client_control_socket);
            if (command.empty()) {
                cerr << "Failed to receive command" << endl;
                break;
            }

            if (command == "quit") {
                cout << "Client requested to quit" << endl;
                break;
            }
            else if (command.substr(0, 3) == "get") {
                string filename = command.substr(4);
                if (handle_get_command(client_control_socket, filename.c_str()) < 0) {
                    cerr << "Failed to handle GET command" << endl;
                }
            } 
            else if (command.substr(0, 3) == "PUT") {
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



int send_data_port(int control_socket, int data_port) {
    char data_port_str[32];
    sprintf(data_port_str, "%d", data_port);
    if (tcp_send(control_socket, data_port_str, sizeof(data_port_str) - 1) < 0) {
        perror("tcp_send");
        return -1;
    }
    return 0;
}

int create_server_socket(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, 100) < 0) {
        perror("listen");
        close(server_socket);
        return -1;
    }

    return server_socket;
}


int create_data_socket(int port) {
    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in data_addr;
    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY;
    data_addr.sin_port = htons(port);

    if (bind(data_socket, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        perror("bind");
        close(data_socket);
        return -1;
    }

    if (listen(data_socket, 100) < 0) {
        perror("listen");
        close(data_socket);
        return -1;
    }

    return data_socket;
}

int accept_client_connection(int server_socket) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int control_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (control_socket < 0) {
        perror("accept");
        return -1;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    cout << "Client connected from " << client_ip << ":" << ntohs(client_addr.sin_port) << endl;

    return control_socket;
}
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


string receive_command(int control_socket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int bytes_received = recv(control_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        cerr << "Failed to receive command" << endl;
        return "";
    }

    buffer[bytes_received] = '\0';
    return string(buffer);
}


void handle_client_commands(int control_socket) {
    char command_buffer[1024];

    while (true) {
        memset(command_buffer, 0, sizeof(command_buffer));
        int bytes_received = tcp_recv(control_socket, command_buffer, sizeof(command_buffer), 0);
        if (bytes_received <= 0) {
            perror("tcp_receive");
            break;
        }

        command_buffer[bytes_received] = '\0';
        cout << "Received command: " << command_buffer << endl;
        
        string command_str = command_buffer;
        istringstream iss(command_str);
        string command;
        iss >> command;

        if (command == "get") {
            string filename;
            iss >> filename;
            //filename.pop_back();
            handle_get_command(control_socket, filename.c_str());
        } else if (command == "put") {
            string filename;
            iss >> filename;
            handle_put_command(control_socket, filename.c_str());
        } else if (command == "ls") {
            handle_ls_command(control_socket);
        } else if (command == "quit") {
            break;
        } else {
            cout << "Unknown command: " << command << endl;
        }
    }
}

int handle_get_command(int control_socket, const char* filename) {
    int file_size = send_file_size(control_socket, filename);
    if (file_size < 0) {
        cerr << "Failed to send file size" << endl;
        return -1;
    }
    cerr << "File size: " << file_size << endl;
    int data_port = rand() % (65535 - 1024 + 1) + 1024;
    if (send_data_port(control_socket, data_port) < 0) {
        cerr << "Failed to send data port" << endl;
        return -1;
    }
    cerr << "Data port: " << data_port << endl;
    int data_socket = create_data_socket(data_port);
    if (data_socket < 0) {
        cerr << "Failed to create data socket" << endl;
        return -1;
    }
    cout << "Accepting connection" << endl;
    int client_data_socket = accept_data_connection(data_socket);
    if (client_data_socket < 0) {
        cerr << "Failed to accept data connection" << endl;
        close(data_socket);
        return -1;
    }
    cout << "Connection accepted" << endl;
    if (send_file(client_data_socket, filename, file_size) < 0) {
        cerr << "Failed to send file" << endl;
        close(client_data_socket);
        close(data_socket);
        return -1;
    }

    close(client_data_socket);
    close(data_socket);
    return 0;
}


int handle_put_command(int control_socket, const char* filename) {
    int file_size = receive_file_size(control_socket);
    if (file_size < 0) {
        cerr << "Failed to receive file size" << endl;
        return -1;
    }
    int data_port = rand() % (65535 - 1024 + 1) + 1024;
    if (send_data_port(control_socket, data_port) < 0) {
        cerr << "Failed to send data port" << endl;
        return -1;
    }
    int data_socket = create_data_socket(data_port);
    if (data_socket < 0) {
        cerr << "Failed to create data socket" << endl;
        return -1;
    }
    int client_data_socket = accept_data_connection(data_socket);
    if (client_data_socket < 0) {
        cerr << "Failed to accept data connection" << endl;
        close(data_socket);
        return -1;
    }

    // Close the data_socket as it's no longer needed
    close(data_socket);

    if (receive_file(client_data_socket, filename, file_size) < 0) {
        cerr << "Failed to receive file" << endl;
        close(client_data_socket);
        return -1;
    }

    close(client_data_socket);
    return 0;
}


int handle_ls_command(int control_socket) {
    // Generate random port number
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

    close(client_data_socket);
    return 0;
}


int send_file_list(int data_socket) {
    cout << "Directed to send file list" << endl;
    DIR *dir;
    struct dirent *entry;
    string file_list;
    if (!(dir = opendir("."))) {
        perror("opendir");
        return -1;
    }
    cout << "Reading Directory" << endl;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            file_list.append(entry->d_name);
            file_list.append("\n");
        }
    }
    closedir(dir);
    cout << "Directory Closed" << endl;

    if (tcp_send(data_socket, file_list.c_str(), file_list.size()) < 0) {
        perror("tcp_send");
        return -1;
    }

    return 0;
}

int send_file(int data_socket, const char* filename, int file_size) {
    // Open the file for reading
    FILE* file = fopen(filename, "rb");
    if (file == nullptr) {
        perror("fopen");
        return -1;
    }

    // Send the file content in chunks
    const int buffer_size = 1024;
    char buffer[buffer_size];
    long long bytes_sent = 0;

    while (bytes_sent < file_size) {
        size_t bytes_to_send = min(static_cast<long long>(buffer_size), file_size - bytes_sent);
        size_t bytes_read = fread(buffer, 1, bytes_to_send, file);

        if (bytes_read < 1) {
            perror("fread");
            fclose(file);
            return -1;
        }

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


int send_file_size(int control_socket, const char* filename) {
    struct stat file_stat;
    //cout << "Filename: " << filename << "." << endl;
    if (stat(filename, &file_stat) < 0) {
        perror("stat");
        return -1;
    }
    int file_size = file_stat.st_size;
    char file_size_str[32];
    sprintf(file_size_str, "%d", file_size);

    if (tcp_send(control_socket, file_size_str, sizeof(file_size_str)-1 ) < 0) {
        perror("tcp_send");
        return -1;
    }
    //cerr << "Filesize sent" << endl;
    return file_size;
}

int receive_file(int data_socket, const char* filename, int file_size) {
    // Open the file for writing
    FILE* file = fopen(filename, "wb");
    if (file == nullptr) {
        perror("fopen");
        return -1;
    }

    // Receive the file content in chunks
    const int buffer_size = 1024;
    char buffer[buffer_size];
    long long bytes_received = 0;

    while (bytes_received < file_size) {
        int bytes_to_receive = min(static_cast<long long>(buffer_size), file_size - bytes_received);
        int bytes_read = tcp_recv(data_socket, buffer, bytes_to_receive, display_progress_bar);

        if (bytes_read < 1) {
            perror("tcp_recv");
            fclose(file);
            return -1;
        }

        fwrite(buffer, 1, bytes_read, file);
        bytes_received += bytes_read;
        display_progress_bar(bytes_received, file_size);
    }

    // Close the file
    fclose(file);
    return 0;
}



int receive_file_size(int control_socket) {
    char file_size_str[32];
    memset(file_size_str, 0, sizeof(file_size_str));
    if (tcp_recv(control_socket, file_size_str, sizeof(file_size_str)) < 0) {
        perror("tcp_recv");
        return -1;
    }

    return atoi(file_size_str);
}

void display_progress_bar(long long current, long long total) {
    const int bar_width = 50;
    float progress = static_cast<float>(current) / total;
    int position = static_cast<int>(bar_width * progress);
    cout << "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < position) {
            cout << "=";
        } else if (i == position) {
            cout << ">";
        } else {
            cout << " ";
        }
    }

    cout << "] " << static_cast<int>(progress * 100.0) << " %\r";
    cout.flush();
}
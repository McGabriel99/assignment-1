#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <dirent.h>
#include "../TCPLib.h"


#define BUFFER_SIZE 1024

using namespace std;

int create_client_socket();
int create_data_socket();
int connect_to_server(const char* server_ip, int port);
int handle_commands(int control_socket);
void send_command(int control_socket, const char* command);
int handle_get_command(int control_socket, const char* filename);
int handle_put_command(int control_socket, const char* filename);
int handle_ls_command(int control_socket);
int receive_file_list(int data_socket);
int receive_file(int data_socket, const char* filename, int file_size);
int receive_file_size(int control_socket);
int send_file(int data_socket, const char* filename, int file_size);
int send_file_size(int control_socket, const char* filename);
void display_progress_bar(long long current, long long total);
void execute_command(int server_socket, const string& command);
const char* server_ip;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <server_ip> <server_port>" << endl;
        return 1;
    }

    server_ip = argv[1];
    int server_port = stoi(argv[2]);

    int control_socket = connect_to_server(server_ip, server_port);
    //cout << "client-side control_socket: " << control_socket << endl;
    handle_commands(control_socket);

    close(control_socket);
    return 0;
}

void execute_command(int server_socket, const string& command) {
    string cmd = command.substr(0, command.find(' '));
    string argument = command.substr(command.find(' ') + 1);

    if (cmd == "ls") {
        // Send the LIST command to the server
        tcp_send(server_socket, cmd.c_str(), cmd.length());
        handle_ls_command(server_socket);
    } else if (cmd == "get") {
        //tcp_send(server_socket, cmd.c_str(), cmd.length());
        handle_get_command(server_socket, argument.c_str());
    } else if (cmd == "put") {
        //tcp_send(server_socket, cmd.c_str(), cmd.length());
        handle_put_command(server_socket, argument.c_str());
    } else if (cmd == "quit"){
        //tcp_send(server_socket, cmd.c_str(), cmd.length());
        send_command(server_socket, "quit");
        exit(0);
    }
    else {
        cout << "Unknown command." << endl;
    }
}

int create_client_socket() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        cerr << "Failed to create socket" << endl;
        exit(EXIT_FAILURE);
    }
    return client_socket;
}

int create_data_socket() {
    return create_client_socket();
}

int connect_to_server(const char* server_ip, int port) {
    int client_socket = create_client_socket();
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    //cout << "Client socket: " << client_socket << endl; 

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Failed to connect to server" << endl;
        exit(EXIT_FAILURE);
    }

    return client_socket;
}

int connect_to_data_socket(int port) {
    int client_socket = create_client_socket();
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    //cout << "Client socket: " << client_socket << endl; 
    bool connected = false;
    for(int i = 0; i < 5; i++) {
        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            //cerr << "Failed to connect to server" << endl;
            //exit(EXIT_FAILURE);
            // If fail to connect, sleep and try again
            sleep(1);
            continue;
        }
        connected = true;
        break;
    }
    if(!connected) {
        cerr << "Failed to connect to server" << endl;
        exit(EXIT_FAILURE);
    }

    return client_socket;
}

int handle_commands(int control_socket) {
    while (true) {
        string command;
        cout << "ftp> ";
        getline(cin, command);

        if (command == "quit" || command == "exit") {
            break;
        }

        execute_command(control_socket, command);
    }
    return 0;
}

void send_command(int control_socket, const char* command) {
    //string command_with_terminator = string(command) + "\r\n";
    string command_with_terminator = string(command);
    tcp_send(control_socket, command_with_terminator.c_str(), command_with_terminator.size(), nullptr);
}



int handle_get_command(int control_socket, const char* filename) {
    string command = "get " + string(filename);
    //cout << command << "." << endl;
    send_command(control_socket, command.c_str());
    cerr << "Recieving file size" << endl;
    int file_size = receive_file_size(control_socket);
    if (file_size == -1) {
        cout << "Error: File not found on the server." << endl;
        return 1;
    }
    cerr << "File size recieved: " << file_size << endl;

    char data_port_s[32];
    // Recieve port number from server
    tcp_recv(control_socket, data_port_s, sizeof(data_port_s) - 1);
    int data_port = stoi(data_port_s);
    cout << "Data Port: " << data_port << endl; 

    int data_socket = connect_to_data_socket(data_port);
    receive_file(data_socket, filename, file_size);
    close(data_socket);

    return 0;
}

int handle_put_command(int control_socket, const char* filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "Error: Unable to open the file." << endl;
        return 1;
    }

    string command = "put " + string(filename);
    send_command(control_socket, command.c_str());

    send_file_size(control_socket, filename);

    int data_socket = create_data_socket();
    streampos file_size;
    file.seekg(0, ios::end);
    file_size = file.tellg();
    file.seekg(0, ios::beg);

    send_file(data_socket, filename, static_cast<int>(file_size));
    close(data_socket);

    return 0;
}

int handle_ls_command(int control_socket) {
    char data_port_s[32];

    // Recieve port number from server
    tcp_recv(control_socket, data_port_s, sizeof(data_port_s) - 1);
    int data_port = stoi(data_port_s);
    //cerr << "Client side data_port: " << data_port << endl;

    // Create a data connection to the server
    int data_socket = connect_to_data_socket(data_port);
    
    // Do ls
    receive_file_list(data_socket);
    close(data_socket);

    return 0;
}

int receive_file_list(int data_socket) {
    char buffer[1024];
    int bytes_received;

    while ((bytes_received = tcp_recv(data_socket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_received] = '\0';
        cout << buffer;
    }

    return 0;
}

int receive_file(int data_socket, const char* filename, int file_size) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "Error: Unable to create the file." << endl;
        return 1;
    }

    char buffer[1024];
    int bytes_received;
    int total_bytes_received = 0;

    while (total_bytes_received < file_size) {
        bytes_received = tcp_recv(data_socket, buffer, sizeof(buffer), display_progress_bar);
        file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;
    }

    cout << "\nFile transfer completed." << endl;
    file.close();
    return 0;
}

int receive_file_size(int control_socket) {
    char file_size_str[32];
    tcp_recv(control_socket, file_size_str, sizeof(file_size_str)-1);
    int  file_size = stoi(file_size_str);
    return file_size;
}

int send_file(int data_socket, const char* filename, int file_size) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "Error: Unable to open the file." << endl;
        return 1;
    }

    char buffer[1024];
    int bytes_read;
    int total_bytes_sent = 0;

    while (total_bytes_sent < file_size) {
        file.read(buffer, sizeof(buffer));
        bytes_read = file.gcount();
        tcp_send(data_socket, buffer, bytes_read, display_progress_bar);
        total_bytes_sent += bytes_read;
    }

    cout << "\nFile transfer completed." << endl;
    file.close();
    return 0;
}

int send_file_size(int control_socket, const char* filename) {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cout << "Error: Unable to open the file." << endl;
        return 1;
    }

    int file_size = static_cast<int>(file.tellg());
    file.close();
    int converted_size = htonl(file_size);
    tcp_send(control_socket, &converted_size, sizeof(converted_size));

    return 0;
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
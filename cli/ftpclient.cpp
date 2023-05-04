#include <iostream>        //for cout, cerr
#include <fstream>         //for ifstream, ofstream
#include <string>          //for string
#include <sys/stat.h>      //for stat and fstat to get file size      
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
using std::getline;
using std::cin;
using std::stoi;
using std::rand;
using std::srand;
using std::min;

int create_client_socket();
int create_data_socket();
int receive_data_port(int control_socket, int data_port);
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
void execute_command(int server_socket, const string& command);
void display_progress_bar(long long current, long long total);
const char* server_ip;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <server_ip> <server_port>" << endl;
        return 1;
    }

    server_ip = argv[1];
    int server_port = stoi(argv[2]);

    int control_socket = connect_to_server(server_ip, server_port);

    cout << "Connected to server " << server_ip << ":" << server_port << endl;

    handle_commands(control_socket);

    close(control_socket);
    return 0;
}


/*
    * Creates a socket for the client
    * @return - the socket file descriptor
*/
int create_client_socket() {

    // Create a socket for client
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        cerr << "Failed to create socket" << endl;
        exit(EXIT_FAILURE);
    }
    return client_socket;
}

/*
    * Creates a socket for the data file transfer for client
    * @return - the socket file descriptor
*/
int create_data_socket() {
    // Create a socket for client, but for data file transfer
    return create_client_socket();
}

/*
    * Receives the data port from the server
    * @param control_socket - the socket to receive the data port from
    * @param data_port - the data port to receive
    * @return - 0 on success, -1 on failure
*/
int receive_data_port(int control_socket, int data_port){
    // Receive the data port from the server
    char data_port_s[32];
    sprintf(data_port_s, "%d", data_port);
    if (tcp_recv(control_socket, data_port_s, sizeof(data_port_s) - 1) < 0){
        perror("tcp_recv");
        return -1;
    }
    return 0;
}

/*
    * Connects to the server
    * @param server_ip - the client's ip address
    * @param port - the port to connect to
    * @return - the socket file descriptor
*/

int connect_to_server(const char* server_ip, int port) {
    // Create a socket for client for connection
    int client_socket = create_client_socket();
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    //cout << "Client socket: " << client_socket << endl; 
    
    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "Failed to connect to server" << endl;
        exit(EXIT_FAILURE);
    }

    return client_socket;
}

/*
    * connects to the server's data socket
    * @param port - the port to connect to
    * @return - the socket file descriptor
*/
int connect_to_data_socket(int port) {
    // Create a socket for client for data file transfer
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

/*
    * handles the commands from the client
    * @param control_socket - the socket to receive the data port from
    * @return - 0 on success, -1 on failure
*/
int handle_commands(int control_socket) {
    while (true) {
        string command;
        cout << "ftp> ";
        getline(cin, command);

        cout << "Sending command: " << command << endl;

        if (command == "quit" || command == "exit") {
            cout << "Received quit command. Exiting..." << endl;
            execute_command(control_socket, command);
            break;
        }
        //for get, put, ls
        execute_command(control_socket, command);
    }
    return 0;
}

/*
    * sends the command to the server
    * @param control_socket - the socket to receive the data port from
    * @param command - the command to send
*/
void send_command(int control_socket, const char* command) {
    // Send command to server
    string command_with_terminator = string(command);
    tcp_send(control_socket, command_with_terminator.c_str(), command_with_terminator.size(), nullptr);
}


/*
    * handles the get command
    * @param control_socket - the socket to receive the data port from
    * @param filename - the file to get from the server
    * @return - 0 on success, -1 on failure
*/
int handle_get_command(int control_socket, const char* filename) {
    // concatenate command and filename
    string command = "get " + string(filename);
    // Send command as a c string to server via control socket
    send_command(control_socket, command.c_str());

    cout << "\n==========Handling get command==========" << endl;
    // Receive file size from server
    int file_size = receive_file_size(control_socket);
    if (file_size == -1) {
        cout << "Error: File not found on the server." << endl;
        return 1;
    }
    cerr << "File size received: " << file_size << endl;

    char data_port_s[32];
    // Receive port number from server
    tcp_recv(control_socket, data_port_s, sizeof(data_port_s) - 1);
    int data_port = stoi(data_port_s);
    cout << "From data Port: " << data_port << endl; 
    // Connect to server's data socket to receive file
    int data_socket = connect_to_data_socket(data_port);
    receive_file(data_socket, filename, file_size);
    cout << "Receiving file: " << filename << endl;
    cout << "==========End of get command==========" << endl;
    close(data_socket);

    return 0;
}

/*
    * handles the put command
    * @param control_socket - the socket to receive the data port from
    * @param filename - the file to put to the server
    * @return - 0 on success, -1 on failure
*/
int handle_put_command(int control_socket, const char* filename) {
    // check if file exists
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "Error: Unable to open the file." << endl;
        return 1;
    }
    //concatenate command and filename
    string command = "put " + string(filename);
    // Send command as a c string to server via control socket
    send_command(control_socket, command.c_str());
    
    cout << "\n==========Handling put command==========" << endl;
    // Send file size to server
    int file_size = send_file_size(control_socket, filename);
    if (file_size < 0){
        cerr << "Failed to send file size" << endl;
        return -1;
    }
    cerr << "File size: " << file_size << endl;

    char data_port_s[32];
    // Receive port number from server
    tcp_recv(control_socket, data_port_s, sizeof(data_port_s) - 1);
    int data_port = stoi(data_port_s);
    cout << "From data Port: " << data_port << endl;
    // Connect to server's data socket to send file
    int data_socket = connect_to_data_socket(data_port);
    cout << "Sending file: " << filename << endl;
    send_file(data_socket, filename, file_size);
    cout << "==========End of put command==========" << endl;
    close(data_socket);

    return 0;
}

/*
    * handles the ls command
    * @param control_socket - the socket to receive the data port from
    * @return - 0 on success, -1 on failure
*/
int handle_ls_command(int control_socket) {
    string command = "ls";
    send_command(control_socket, command.c_str());

    char data_port_s[32];

    // Recieve port number from server
    tcp_recv(control_socket, data_port_s, sizeof(data_port_s) - 1);
    int data_port = stoi(data_port_s);
    //cerr << "Client side data_port: " << data_port << endl;

    // Create a data connection to the server
    int data_socket = connect_to_data_socket(data_port);
    
    // execute ls command
    cout << "\n==========Receiving file list:==========" << endl;
    receive_file_list(data_socket);
    cout << "==========End of file list==========" << endl;
    close(data_socket);

    return 0;
}

/*
    * receive the file list from the server called by ls command
    * @param data_socket - the socket to receive the data port from
    * @return - 0 on success, -1 on failure
*/
int receive_file_list(int data_socket) {
    // buffer to store the file list
    char buffer[BUFFER_SIZE];
    int bytes_received;
    // receive the file list from the server and print it out
    while ((bytes_received = tcp_recv(data_socket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_received] = '\0';
        cout << buffer;
    }

    return 0;
}


/*
    * receive the file from the server called by get command
    * @param data_socket - the socket to receive the data port from
    * @param filename - the file to receive
    * @param file_size - the size of the file to receive
    * @return - 0 on success, -1 on failure
*/
int receive_file(int data_socket, const char* filename, int file_size) {
    // Create a file to store the received file
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "Error: Unable to create the file." << endl;
        return 1;
    }
    // buffer to store the received file
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int total_bytes_received = 0;
    // receive the file from the server and write it to the file
    while (total_bytes_received < file_size) {
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
    * receive file size from the server called by get command
    * the redundant function is to make sure the file size is received
      before the file transfer starts, and also if file size is needed to be
      accessed by itself, instead of calling the whole receive_file function
*/
int receive_file_size(int control_socket) {
    // file size is sent as a string
    char file_size_str[32];
    // receive the file size from the server
    tcp_recv(control_socket, file_size_str, sizeof(file_size_str)-1);
    // convert the string to int
    int file_size = stoi(file_size_str);
    return file_size;
}

/*
    * send file to the server called by put command
    * @param data_socket - the socket to receive the data port from
    * @param filename - the file to send
    * @param file_size - the size of the file to send
    * @return - 0 on success, -1 on failure
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
    cout << "\nFile transfer completed." << endl;
    fclose(file);
    return 0;
}

/*
    * send file size to the server called by put command
    * the redundant function is to make sure the file size is sent
      before the file transfer starts, and also if file size is needed to be
      accessed by itself, instead of calling the whole send_file function
    @param control_socket - the socket to receive the data port from
    @param filename - the file to send
    @return - 0 on success, -1 on failure
*/
int send_file_size(int control_socket, const char* filename) {
    // stat the file to get its size
    struct stat file_stat;
    if (stat(filename, &file_stat) < 0) {
        perror("stat");
        return -1;
    }
    // convert the file size to string
    int file_size = file_stat.st_size;
    char file_size_str[32];
    sprintf(file_size_str, "%d", file_size);
    // send the file size to the server
    if (tcp_send(control_socket, file_size_str, sizeof(file_size_str)-1 ) < 0) {
        perror("tcp_send");
        return -1;
    }

    return file_size;
}

/*
    * execute command from the user
    * @param server_socket - the socket to receive the data port from
    * @param command - the command to execute
    * @return - 0 on success, -1 on failure
*/
void execute_command(int server_socket, const string& command) {
    string cmd = command.substr(0, command.find(' '));
    string argument = command.substr(command.find(' ') + 1);

    if (cmd == "ls") {
        handle_ls_command(server_socket);
    } else if (cmd == "get") {
        handle_get_command(server_socket, argument.c_str());
    } else if (cmd == "put") {
        handle_put_command(server_socket, argument.c_str());
    } else if (cmd == "quit"){
        // no function needed for quit, just send the command to the server
        send_command(server_socket, "quit");
        close(server_socket);
    }
    else {
        cout << "Unknown command." << endl;
    }
}

/*
    * display the progress bar for file transfer
    * @param current - the current size of the file
    * @param total - the total size of the file
*/
void display_progress_bar(long long current, long long total) {
    const int bar_width = 50;
    float progress = static_cast<float>(current) / total;
    int position = static_cast<int>(bar_width * progress);
    cout << "Transferring file: ";
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
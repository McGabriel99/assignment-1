# Protocol Design Pseudocode
## This document walks through the step-by-step processes of the client and server programs.
### Server sets up port, waits.
```
create_server_socket(server_port)
    # Create raw socket with no arguments
    server_socket = socket()

    # Bind socket to port number
    bind(server_socket, server_port)

    # Wait and listen for connection
    listen(server_socket)

    return server_socket
```

### The client creates its own socket to connect to the server.
```
create_client_socket()
    client_socket = socket()
    return client_socket
```

### The client now attempts to establish a control connection between the client and server. The client needs to know the server's IP address and the port number.
```
connect_to_server(server_address, server_port)
    # Create a socket
    client_socket = create_client_socket()

    # Attempt to connect the client socket to the server socket
    if connect(client_socket, server_address, server_port) == False:
        exit("Failed to connect to server")
    return client_socket
```

### If successfully sent, the server will accept the client connection. This completes the 3-way handshake. In FTP, the default control connection port number is 21.
```
accept_client_connection(client_address, client_socket)
    if accept(server_socket, client_address, client_socket) == False
        exit("Failed to create control connection")
    return client_socket
```

### Whenever a file is sent, the server randomly generates a second port number, and sends it over the control link. The client then acknowledges the port number. Finally, the server establishes the data connnection. After the file is sent, the data sender automatically closes the data connection. This is very similar to the 3-way handshake from above. For FTP in particular, the default data connection port number is 20. Example below:
```
create_data_socket(data_port)
    data_socket = socket()
```

```
send_data_port(control_socket, data_port)
    if send(control_socket, data_port) == False
        error("Failed to send data port")
    return data_port
```

```
receive_data_port(control_socket, data_port)
    if receive(control_socket, data_port) == False
        error("Failed to retrieve data port")
    return data_port
```

```
accept_data_connection(client_address, data_port)
    if accept(server_socket, client_address, data_port) == False
        exit("Failed to create data connection")
    return data_port
```

### Server waits for client to send orders, the 4 different commands are listed below. Both get() and put() share the same underlying functions, only that the direction between sender and receiver are switched.
1. get(file) - Client downloads file from server.

2. put(file) - Client uploads their own file to the server.

```
send_file(data_socket, filename)
    file = open(filename)
    bytes_sent = 0
    buffer[BUFFER_SIZE]

    while(bytes_sent < size(file))
        buffer = read(file, BUFFER_SIZE)
        send(data_socket, buffer)

    close(file)
```

```
receive_file(data_socket, filename, filesize)
    file(filename)
    bytes_received = 0
    buffer<BUFFER_SIZE>

    while(bytes_received < filesize)
        buffer = receive(data_socket)
        file.write(buffer)

    close(file)
```

3. ls() - Lists the contents in the current directory that the server is in.
```
send_file_list(data_socket)
    dir = <CURRENT_DIRECTORY>
    entry, file_list = ""

    while((entry = read(dir)) != NULL)
        file_list.append(entry)
    
    close(dir)
    send(data_socket, file_list)
```

4. quit() - Client sends a command to close the control link, before closing their end. Server then receives message and closes their end of the control link.
```
close_client_socket(client_socket)
    close(client_socket)
```

```
close_server_socket(server_socket)
    close(server_socket)
```
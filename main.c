#include "includes/definitions.h"
#include <stdio.h>

/* Global variables */
int server_fd;
FILE* fptr;

int client_counter;
int banned_count; 

// Arrays for holding client data
int client_sockets[MAX_CLIENTS];
char* client_names[MAX_CLIENTS];
PublicKey client_public_keys[MAX_CLIENTS];
char* banned_clients[MAX_BANNED];

struct client_data *server;
pthread_mutex_t client_sockets_mutex = PTHREAD_MUTEX_INITIALIZER;

void registerClient(struct client_data *client)
{
    char* registerString = (char*)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(registerString, BUFFER_SIZE, "=====\t Welcome to %s chatroom! \t=====\nPlease enter your username: ", server->name); 
    
    //char* registerString = "===\tWelcome to my server!\t===\nPlease enter your username: ";
    int _r = send(client->socket, registerString, strlen(registerString), 0);
    if (_r < 0) perror("[!] Couldn't reach a guest.\n" NC); 
    
    char buffer[BUFFER_SIZE];
    int dataread = read(client->socket, buffer, BUFFER_SIZE - 1);
    if (dataread > 0)
    {
        buffer[dataread] = '\0';
        // Remove the new lines
        char* pos = strchr(buffer, '\n');
        if (pos != NULL) *pos = '\0';
        int isBanned = is_banned(buffer, client_names, client_counter, fptr); 
        if (isBanned)
        {
            char* banMsg = "You are banned from this server.\n";
            send(client->socket, banMsg, strlen(banMsg), 0);
            close(client->socket);
            free(client->name);
            free(client);
            pthread_exit(NULL);
        }
        // Assigning data to the client
        client->name = strdup(buffer);
        client_names[client->id - 1] = client->name;
        fprintf(fptr, "[*] %s [%d] has joined to the server.\n", client->name, client->id);
        
        client->pubKey = generatePublicKey();
        client_public_keys[client->id - 1] = client->pubKey;
        client->privKey = generatePrivateKey(client->pubKey);

        // Logging the user's keys
        fprintf(fptr, "[*] %s's encryption -> PubK:{e:%d; N:%d; phi:%d}, PrivK:{key:%d}.\n", 
            client->name, client->pubKey.e, client->pubKey.N, client->pubKey.phi, client->privKey.key);
    }
    free(registerString);
}

// Signal handler for secure shutdown
void handle_shutdown()
{
    printf("\nShutting down the server...\n");
    if (fptr != NULL)
    {
        fprintf(fptr, ">>\tServer has shut down.\t<<");
        fclose(fptr);
        printf("\t- Logs.txt file was closed");
    }
    close(server_fd);
    exit(0);
}

int getClientSocket(char* clientName)
{
    pthread_mutex_lock(&client_sockets_mutex);
    for(int i = 0; i < client_counter; i++)
    {
        if (client_sockets[i] != -1 && strcmp(client_names[i], clientName) == 0) // Checks if the socket is used (not -1)
        {
            int socket = client_sockets[i];
            pthread_mutex_unlock(&client_sockets_mutex);
            return socket;
        }
    }
    pthread_mutex_unlock(&client_sockets_mutex);
    return -1;
}

void handleClientCommands(char* command, struct client_data *client, int offset)
{
    char* helpMsg = "Welcome to client commands! Here are the basic commands you have:\n\t- h: Displays help.\n\t- w <client_name> <msg>: Whispers to that client.\n";
    char* errorMsg = "\n[!] Command not found.\n";
    char* userNotFoundMsg = "\n[!] User not found.\n";
    //char* missingArgsMsg = "\n[!] Missing arguments for command.\n";

    int _r;
    char* args = command + offset + 2; // Skip "$w "
    char* recipient = strtok(args, " ");
    char* msg = strtok(NULL, "");

    switch (command[offset]) 
    {
        case 'h':   // Help pannel
            _r = send(client->socket, helpMsg, strlen(helpMsg), 0);
            if (_r < 0) perror(RED "[!] Couldn't send help message to client that used the command: \"$h\".\n" NC); 
            break;

        case 'w':   // Whisper <client> <msg>
            if (recipient == NULL || msg == NULL) 
            {
                _r = send(client->socket, errorMsg, strlen(errorMsg), 0);
                if (_r < 0) perror(RED "[!] Couldn't send error message to client due to missing arguments.\n" NC);
                return;
            }

            int receiverSocket = getClientSocket(recipient);
            if (receiverSocket == -1) {
                 _r = send(client->socket, userNotFoundMsg, strlen(userNotFoundMsg), 0);
                if (_r < 0) perror("[!] Couldn't send user not found message to client.\n");
                return;
            }

            size_t msg_size = strlen(client->name) + strlen(msg) + 12; // \r\n[client->name] msg\r\n
            char* formatted_msg = (char*)malloc(msg_size);
            snprintf(formatted_msg, msg_size, "\r(%s): %s", client->name, msg);

            _r = send(receiverSocket, formatted_msg, strlen(formatted_msg), 0);
            if (_r < 0) perror("[!] Couldn't send the whisper to the destinatary.");
            
            sendPrefixSymbol(receiverSocket, "> ");
            fprintf(fptr, "[ ** ] Whisper from %s to %s", client->name, recipient);
            free(formatted_msg);
            break;

        default:
            _r = send(client->socket, errorMsg, strlen(errorMsg), 0);
            if (_r < 0) printf(RED "[!] Couldn't send error message to client that used the command: \"%c\"" NC, command[1]);
            break;
    }
}

void sendMessage(char *msg, struct client_data *client)
{
    int client_name_len = strlen(client->name);

    /* The way of calculating the +4 offset:
    for(int i=0; i < strlen(msg);i++)
    {
        if (msg[i] == '$') printf("%d", i+1);
    }*/

    int offset = client_name_len+4;
    if (msg[offset] == '$')
    {
        /// msg = "\r[client->name] $command\0"
        handleClientCommands(msg, client, offset+1);
    }
    else
    {
        int sender_id = client->id;
        pthread_mutex_lock(&client_sockets_mutex);

        for(int i = 0; i < client_counter; i++)
        {
            if (i == sender_id-1) continue; // The sender does not receive its own message!
            if (client_sockets[i] != -1) // Checks if the socket is used (not -1)
            {
                // Encrypt the message using each client's public key
                //msg = encrypt(msg, client_public_keys[i]);

                int _r = send(client_sockets[i], msg, strlen(msg), 0);
                if (_r < 0) perror(RED "[!] Couldn't send message to client.\n" NC); 
            
                sendPrefixSymbol(client_sockets[i], "> "); 
            }
        }
        pthread_mutex_unlock(&client_sockets_mutex);
    }
    free(msg);
}

// Handling client communication
void *handleClient(void *data)
{
    struct client_data *client = (struct client_data *)data;
    int new_socket = client->socket;
    int client_id = client->id;
    
    registerClient(client);
    char* client_name = client->name;

    char buffer[BUFFER_SIZE];
    int dataread;

    sendPrefixSymbol(new_socket, "> ");
    
    while ((dataread = read(new_socket, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[dataread] = '\0';

        // Decrypting the received message buffer

        size_t msg_size = strlen(client_name) + strlen(buffer) + 6; // "[name] msg\0"
        char* msg = (char*)malloc(msg_size);

        snprintf(msg, msg_size, "\r[%s] %s", client_name, buffer); // msg := "\r[client_name] message\0"
        
        // Send the message to all users, except you.
        sendMessage(msg, client);

        // Send the user's prompt again
        sendPrefixSymbol(new_socket, "\r> ");
        
        fprintf(fptr, "> %s [%d] >>> %s\n", client_name, client_id, msg);
    }

    close(new_socket);

    // Set unused sockets in the array client_sockets[] to -1
    pthread_mutex_lock(&client_sockets_mutex);
    client_sockets[client_id - 1] = -1;
    pthread_mutex_unlock(&client_sockets_mutex);

    free(client->name); // Free duplicated string (see strdup in registerClient())
    free(client);

    return NULL;
}

// A wall is a message sent to every client
void sendWall(char *msg)
{
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), GREEN "\n[%s] %s\n" NC, server->name, msg);

    // Server ID is -1, thus won't send to himself
    sendMessage(buffer, server);

    printf(GREEN "Wall message sent to every client.\n" NC);
}

void kickClient(int client_socket, char* client_name)
{
    char* msg = "Client has been kicked from the server.";
    sendWall(msg);

    char* warning = "\r\nYou have been kicked from the server.\n";
    int _r = send(client_socket, warning, strlen(warning), 0);
    if (_r < 0) error("Couldn't kick a client.");

    close(client_socket);
    pthread_mutex_lock(&client_sockets_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_names[i] != NULL && strcmp(client_names[i], client_name) == 0) {
            free(client_names[i]);
            client_names[i] = NULL;
        }
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = -1;
        }
    }

    client_counter--;

    pthread_mutex_unlock(&client_sockets_mutex);

    printf("End of kicking execution.");
}

void banClient(char* client_name) {
    if (banned_count < MAX_BANNED) {
        banned_clients[banned_count++] = strdup(client_name);
        printf("Client %s has been banned.\n", client_name);

        // Disconnect the client if they are currently connected
        for (int i = 0; i < client_counter; i++) {
            if (client_names[i] && strcmp(client_names[i], client_name) == 0) {
                int client_socket = getClientSocket(client_name);
                kickClient(client_socket, client_name);
            }
        }
    } else {
        printf("Ban list is full.\n");
    }
    printf("End of banning execution.");
}

void *handleAdminPannel(char *arg)
{
    char command[BUFFER_SIZE];
    char* args = command+5;
    printf("$ ");
    while (fgets(command, sizeof(command), stdin) != NULL) {
        // Remove \0 character
        command[strcspn(command, "\n")] = '\0';
        
        if (strcmp(command, "clear_logs") == 0) 
        {
            clearLogs(fptr);
            printf("Logs cleared.\n");
        }
        else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0)
        {
            handle_shutdown();
        }
        else if (strncmp(command, "kick", 4) == 0)
        {
            char* client_name = args;
            int client_socket = getClientSocket(client_name);
            if (client_socket != -1) 
            {
                kickClient(client_socket, client_name);
            } else 
            {
                printf("Client not found: %s\n", client_name);
            }
        }
        else if (strncmp(command, "ban", 3) == 0)
        {
            char* client_name = command+4;
            banClient(client_name);
        }
        else if (strcmp(command, "show_bans") == 0)
        {
            printf("This is the list of the banned clients: \n");
            for(int i = 0; i < banned_count; i++)
            {
                printf("\t-%s\n", banned_clients[i]);
            }
        }
        // Notice the strNcmp
        else if (strncmp(command, "wall", 3) == 0)
        {
            // The numbers 3 and 5 are used for displaying the message properly
            // 3 => Only checks if "wall" is entered
            // 5 => The message ommits the first 4 chars ("wall ")
            char* wall_msg = command+5;
            printf("Sent wall message: %s\n", wall_msg);
            sendWall(wall_msg);
        }
        else if (strcmp(command, "clear") == 0)
        {
            system("clear");
        }
        else if (strcmp(command, "help") == 0)
        {
            helpPannel();
        }
        else {
            printf("Unknown command: %s\n", command);
        }
        printf("$ ");
    }
    
    return NULL;
}

int main(int argc, char **argv)
{
    server = (struct client_data *)malloc(sizeof(struct client_data));
    server->socket = 0;
    server->id = MAX_CLIENTS+1;
    server->name = SERVER_NAME;

    struct client_data *client;
    struct sockaddr_in address; // Holds sockaddress info
    int opt = 1;                // Option for setsockopt()
    int addrlen = sizeof(address);

    // Setting up SIGINT
    signal(SIGINT, handle_shutdown);

    // Creating the socket fd
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        error("Socket failed");
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;           // Set address family to Internet
    address.sin_addr.s_addr = INADDR_ANY;   // accepts connections from any IP address
    address.sin_port = htons(SERVER_PORT);  // Convert port number to network byte order

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        error("Bind failed");
    }
    
    if (listen(server_fd, 3) < 0)
    {
        error("Listen failed");
    }

    // Handling logs
    fptr = fopen("Logs.txt", "a");
    if (fptr == NULL) {
        error("Error opening log file");
    }
    if (argc > 1 && argv[1][0] == 'c') clearLogs(fptr);
    fprintf(fptr, "\n===  Starting server on port %d ===\n", SERVER_PORT);

    printf("Server is listening on port %d\n", SERVER_PORT);

    // Creating the handleAdminCommands thread
    pthread_t command_thread;
    if (pthread_create(&command_thread, NULL, (void *(*)(void *))handleAdminPannel, NULL) != 0)
    {
        free(client);
        error("Thread creation for command handling failed");
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_sockets[i] = -1;
    }
    printf("=====\t Welcome to %s! \t=====\n", server->name);
    puts("Type <help> to display the manual.");

    //testEncryptionDecryption();

    /* Creating server public keys: */
    PublicKey Server_Key = generatePublicKey();
    server->pubKey = Server_Key;
    
    while (1)
    {
        client = (struct client_data *)malloc(sizeof(struct client_data));
        if (client == NULL) error("Malloc failed");

        client->socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t *)&addrlen);
        if (client->socket < 0) {
            free(client);
            error("Accept failed");
        }

        client->id = ++client_counter;  // Assign a unique ID to the client

        if (client_counter > MAX_CLIENTS) {
            close(client->socket);
            free(client);
            client_counter--;
            continue;
        }
        pthread_mutex_lock(&client_sockets_mutex);
        client_sockets[client->id - 1] = client->socket;
        pthread_mutex_unlock(&client_sockets_mutex);

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handleClient, (void*)client) != 0) {
            free(client);
            error("Thread creation failed");
        }

        pthread_detach(client_thread);
    }
    return 0;
}
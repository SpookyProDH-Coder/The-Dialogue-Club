#include "definitions.h"
#include <stdio.h>


void helpPannel()
{
    printf("Welcome to the help pannel.\n");
    printf("%s is a served-based and multi-threaded chatroom which allows multiple clients interact each other in real time.\n", SERVER_NAME);
    printf("The list of available commands:\n");

    printf("\t- help\t- Displays this list.\n");
    printf("\t- clear_logs\t- Clears the Logs.txt file.\n");
    printf("\t- wall\t- Send a server message to every client.\n");
    printf("\t- kick <client>\t- Reject an user given it's name.\n");
    printf("\t- ban <client>\t- Ban a client given it's name.\n");
    printf("\t- show_bans\t- Displays the list of current banned users.\n");
    printf("\t- clear\t- Clears the current terminal.\n");
    printf("\t- exit | quit\t- Stop this server.\n");
    printf("In order to connect a client to this server, you can use the following command: \n");
    printf("\t- nc localhost %d\n", SERVER_PORT);
    printf("This is an alpha version of the chatroom. Future updates will be delivered.\n");
    printf("This chat-room was not meant to be " RED "secure" NC ". Use it on your own risk.\n" NC);
}

void clearLogs(FILE* logs)
{
    logs = freopen("Logs.txt", "w", logs);
    if (logs == NULL) {
        perror(RED "[!] Error cleaning the file" NC);
        exit(1);
    }
    fflush(logs);
}

void sendPrefixSymbol(int client_socket, char* symbol)
{
    int _r = send(client_socket, symbol, strlen(symbol), 0);
    if (_r < 0) perror(RED "[!] Couldn't send command prompt symbol to client.\n" NC); 
}

void error(const char *msg)
{
    perror(msg);
    handle_shutdown();
}

int is_banned(char* client_name, char** banned_clients, int banned_count, FILE* logs)
{
    if (client_name == NULL)
    {
        perror("[!] The client_name is NULL, ending is_banned execution");
        return 0;
    }
    for(int i = 0; i < banned_count; i++)
    {
        //printf("Comparing %s with %s\n", banned_clients[i], client_name);
        if (banned_clients[i] != NULL && strcmp(banned_clients[i], client_name) == 0)
        {
            fprintf(logs, "[!] Banned user [%s] attempted to join the server.\n", client_name);
            return 1;
        }
    }
    return 0;
}
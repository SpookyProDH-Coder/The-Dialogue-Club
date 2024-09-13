#ifndef DEFINITIONS_CHATROOM_H
#define DEFINITIONS_CHATROOM_H

// Compiler Header inclusions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>     // For close() function
#include <arpa/inet.h>
#include <sys/types.h>  // Socket data types
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>       // Encryption functions
#include <string.h>


/* Server pre-defined values */
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024    // Read data buffer
#define MAX_CLIENTS 100
#define MAX_BANNED 100
#define SERVER_NAME "The Dialogue Bar"  // Funny name

/* Color definitions using ANSII char-codes */
#define NC "\e[0m"
#define RED "\e[1;31m"    // Also \e[31m
#define GREEN "\e[32m"
#define BLUE "\e[34m"

typedef struct {
    int N;
    int e;
    int phi; // Used for generatePrivateKey()
} PublicKey;

typedef struct {
    int key;
} PrivateKey;

// Structure to parse client data to the client handler
struct client_data {
    int socket;
    int id;
    char* name;

    PrivateKey privKey;
    PublicKey pubKey;
};

/* Function declarations */

// handler.c
void registerClient(struct client_data *client);

// helpers.c
void helpPannel();
void clearLogs(FILE* logs);
void sendPrefixSymbol(int client_socket, char* symbol);
void handle_shutdown();
void error(const char *msg);
int is_banned(char* client_name, char** banned_clients, int banned_count, FILE* logs);

// encryption.c
int isPrime(int n);
int extended_gcd(int a, int b, int* x, int* y);
int getTotient(int p, int q);
int mod_exp(int base, int exponent, int modulus);
int mod_inverse(int a, int phi);
PrivateKey generatePrivateKey(PublicKey PK);
PublicKey generatePublicKey();
char* encrypt(char* pt, PublicKey PK);
char* decrypt(char* ct, PrivateKey PrivKey, PublicKey pubKey) ;
void testEncryptionDecryption();

#endif
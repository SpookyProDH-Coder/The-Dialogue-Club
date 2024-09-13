#include "definitions.h"

int isPrime(int n) 
{
    if (n <= 1) return 0;

    if (n == 2) return 1;
    if (n % 2 == 0) return 0;

    int max_div = (int)floor(sqrt(n));

    for (int i = 3; i <= max_div; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int extended_gcd(int a, int b, int* x, int* y) 
{
    if (a == 0) 
    {
        *x = 0;
        *y = 1;

        return b;
    }

    int x1, y1;
    int gcd = extended_gcd(b % a, a, &x1, &y1);

    *x = y1 - (b / a) * x1;
    *y = x1;

    return gcd;
}

int getTotient(int p, int q)
{
    return (p - 1) * (q - 1);
}

int mod_exp(int base, int exponent, int modulus)
{
    int result = 1;
    base %= modulus;
    while (exponent > 0) {
        if (exponent % 2 == 1) {
            result = (result * base) % modulus;
        }
        exponent >>= 1;
        base = (base * base) % modulus;
    }
    return result;
}

int mod_inverse(int a, int phi) 
{
    int m0 = phi, t, q;
    int x0 = 0, x1 = 1;

    if (phi == 1) return 0;

    while (a > 1) 
    {
        q = a / phi;
        t = phi;

        phi = a % phi, a = t;
        t = x0;

        x0 = x1 - q * x0;
        x1 = t;
    }

    if (x1 < 0) x1 += m0;

    return x1;
}

PrivateKey generatePrivateKey(PublicKey PK) 
{
    PrivateKey _priv;
    _priv.key = mod_inverse(PK.e, PK.phi);
    return _priv;
}

PublicKey generatePublicKey() 
{
    srand((unsigned)time(NULL));

    int p = 0, q = 0;
    // p & q has to pertain to the interval [100, 200]

    while (!isPrime(p)) 
    {
        p = rand() % 100 + 100;
    }

    while (!isPrime(q) || q == p) 
    {
        q = rand() % 100 + 100;
    }
    
    int N = p * q;
    int phi = getTotient(p, q);
    int e = 65537;

    PublicKey pk = {N, e, phi};
    return pk;
}

char* encrypt(char* pt, PublicKey PK) 
{
    // Allocate the message onto the heap
    size_t msg_buffer = strlen(pt) * 12;
    char* msg = (char*)malloc(msg_buffer);

    char cipher[12];

    for (size_t i = 0; i < strlen(pt); i++) 
    {
        int _r = mod_exp((int)pt[i], PK.e, PK.N);
        snprintf(cipher, sizeof(cipher), "%d ", _r);
        strcat(msg, cipher);
    }
    return msg;
}

char* decrypt(char* ct, PrivateKey PrivKey, PublicKey pubKey) 
{
    // Allocate the message onto the heap
    size_t msg_buffer = strlen(ct);
    char* pt = (char*)malloc(msg_buffer);

    // Split each character with a space
    char* token = strtok(ct, " "); 
    int index = 0;

    while (token != NULL) 
    {
        // Token to integer
        int cipher = atoi(token);

        int _r = mod_exp(cipher, 
        PrivKey.key, pubKey.N);  // Decrypt using the private key
        char letter = (char) _r; 
        pt[index++] = letter;
        // Move to the next token
        token = strtok(NULL, " ");
    }
    
    // Null-terminate the decrypted message
    pt[index] = '\0';  
    return pt;
}

// Function used for testing encryption and decryption
void testEncryptionDecryption() {
    char *message = "Test message";
    PublicKey pubKey = generatePublicKey();
    PrivateKey privKey = generatePrivateKey(pubKey);

    char *encryptedMessage = encrypt(message, pubKey);
    char *decryptedMessage = decrypt(encryptedMessage, privKey, pubKey);

    printf("Original message: %s\n", message);
    printf("Encrypted message: %s\n", encryptedMessage);
    printf("Decrypted message: %s\n", decryptedMessage);

    free(encryptedMessage);
    free(decryptedMessage);
}
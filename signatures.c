#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "hash.h"


int main(int argc, char* argv[]) {

    printf("Creating Keypair:\n");
    RSA* keypair = RSA_generate_key(2048,3,NULL,NULL);

    //Create structures to seperate keys
    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    //Extract data out of RSA structure 
    PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, keypair);

    //Get length of data
    size_t pri_len = BIO_pending(pri);
    size_t pub_len = BIO_pending(pub);

    //Prepare char buffers for keys
    char *pri_key = malloc(pri_len + 1);
    char *pub_key = malloc(pub_len + 1);

    //Read into buffers
    BIO_read(pri,pri_key,pri_len);
    BIO_read(pub, pub_key,pub_len);

    //Terminate strings
    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    printf("public length: %zu\n", pub_len);
    printf("private length: %zu\n", pri_len);

    printf("\n%s\n%s\n", pri_key, pub_key);


    char msg[2048/8];
    printf("Message to encrypt: ");
    fgets(msg, 2048/8, stdin);
    msg[strlen(msg)] = '\0';

    unsigned char data[32];
    hash256(data,msg);

    ////////////////////////////////////////////////////////////////////////////////////
    //sign the data
    ////////////////////////////////////////////////////////////////////////////////////
    unsigned char* sig = NULL;
    unsigned int sig_len = 0;

    sig = malloc(RSA_size(keypair));
    if(NULL == sig) {printf("didn't work!\n");};

    int rc = RSA_sign(NID_sha256,data,32,sig,&sig_len,keypair);
    if(rc != 1) {printf("ERROR SIGNING!\n");};

    char asci_sig[513] = {0};
    printf("SIGNATURE:\n");
    for(int i = 0; i < 256; i++) {
        printf("%02x", sig[i]);
        char buf[3] = {0};
        sprintf(buf,"%02x",sig[i]);
        strcat(asci_sig, buf);
    }
    printf("\n");

    free(sig);
    printf("ASCI SIG:%s\n", asci_sig);
    ////////////////////////////////////////////////////////////////////////////////////
    //verify the data
    ////////////////////////////////////////////////////////////////////////////////////
    // asci_sig -> signature[256]
    // pub_key -> 
    // data ->

    unsigned char signature[256];
    char* pointer = asci_sig;
    //extract sig from hex asci
    for(int i = 0; i < 256; i++) {
        unsigned int value;
        sscanf(pointer, "%02x", &value);
        //printf("%02x", value);
        pointer = pointer + 2;
        signature[i] = value;
    }

    printf("\nsize of key: %lu\n", strlen(pub_key) + 1);
    printf("%s\n", pub_key);

    BIO *bio = BIO_new_mem_buf((void*)pub_key, strlen(pub_key));
    RSA *rsa_pub = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);

    rc = RSA_verify(NID_sha256, data,32,signature,256,rsa_pub);
    if(rc != 1) printf("ERROR VERIFYING!\n"); else printf("VERIFIED!\n");

    ////////////////////////////////////////////////////////////////////////////////////






    char* encrypt = malloc(RSA_size(keypair));
    int encrypt_len;
    char* err = malloc(130);

    if((encrypt_len = RSA_public_encrypt(strlen(msg+1),(unsigned char*)msg,
    (unsigned char*)encrypt, keypair,RSA_PKCS1_OAEP_PADDING)) ==  -1) {
        ERR_load_CRYPTO_strings();
        ERR_error_string(ERR_get_error(),err);
        fprintf(stderr, "Error encrypting message: %s\n", err);
    }

    

    char *decrypt = malloc(RSA_size(keypair));
    if(RSA_private_decrypt(encrypt_len, (unsigned char*)encrypt, (unsigned char*)decrypt,
    keypair, RSA_PKCS1_OAEP_PADDING) == -1) {
        ERR_load_crypto_strings();
        ERR_error_string(ERR_get_error(), err);
        fprintf(stderr, "Error decrypting message: %s\n", err);
        } 
    else{
        printf("Decrypted message: %s\n", decrypt);
        }



    

    return 0;
}
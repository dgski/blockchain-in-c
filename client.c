#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>


#include "client_queue.h"
#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"
#include "data_containers/linked_list_string.h"
#include "hash.h"


transaction_queue*  main_queue;
pthread_t network_thread;
strlist* other_nodes;

RSA* your_keys;
char* pub_key;
char* pri_key;


void* send_new(c_transaction* in_trans) {
    if(in_trans->status == 0) {

        int sock_out = nn_socket (AF_SP, NN_PUSH);
        assert (sock_out >= 0);
        int timeout = 50;
        assert (nn_setsockopt(sock_out, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);
        sleep(1);


        printf("\nSending: %s\n", in_trans->message);
        strli_node* current = other_nodes->head;

        for(int i = 0; i < other_nodes->length; i++) {
            int eid = nn_connect (sock_out, current->value); 
            assert(eid >= 0);
            printf("Announcing to: %s, ", current->value);
            int bytes = nn_send (sock_out, in_trans->message, strlen(in_trans->message), 0);
            printf("Bytes sent: %d\n", bytes);
            current = current->next;
            nn_shutdown(sock_out,eid);
        }

        printf("Done sending transaction!\nb-in-c>\n");
        in_trans->status = 1;
        //nn_shutdown(sock,0);
    }
    return NULL;
}

void* update_status(c_transaction* in_trans) {
    if(in_trans->status == 0) {

        //printf("Sending: %s\n", in_trans->message);
        //Send function goes here
        in_trans->status = 1;
    }
    return NULL;
}

void* check_network(){
    
    while(true) {
        //1. Send new transactions to nodes
        queue_map(main_queue, send_new);
        //2. Request blockchain status of transactions
        
        //3. Recieve new data
        //4. Update transactions status
        queue_map(main_queue, update_status);
        //5. Wait short amount to prevent spamming
        sleep(2);
    }
}


void display_help() {
    printf("Help/Command List: 'h'\n");
    printf("Transactions List: 't'\n");
    printf("Post New Transaction: 'n sender reciever amount'\n");
    printf("Quit program: 'q'\n");
    return;
}

void quit_program() {
    printf("Shutting down. Good bye!\n");
    exit(0);
}

//bool val_trans_format(char* sender, char* recipient, char* amount) {
bool val_trans_format(char* recipient, char* amount) {

    /*
    for(int i = 0; i < strlen(sender); i++) {
        if(sender[i] == ':') {
            printf("You cannot use ':' in sender's or reciever's names.\n");
            return false;
        }
    }*/
    for(int i = 0; i < strlen(recipient); i++) {
        if(recipient[i] == ':') {
            printf("You cannot use ':' in senders or recipients names.\n");
            return false;
        }
    }

    if(isnumber(atoi(amount))) {
        printf("Enter a valid number amount to transfer.\n");
        return false;
    }

    return true;
}

//Hash and sign the given message
int message_signature(char* output, char* message, RSA* keypair) {




    //Hash the message
    unsigned char data[32];
    hash256(data,message + 2);

    //Print the hash
    printf("HASHVALUE:\n");
    for(int i= 0; i < 32; i++)
        printf("%02x", data[i]);
    printf("\n");

    //Prepare signature buffer
    unsigned char* sig = malloc(RSA_size(keypair));
    unsigned int sig_len = 0;
    if(sig == NULL) return 0;

    //Create signature
    int rc = RSA_sign(NID_sha256,data,32,sig, &sig_len,keypair);
    if(rc != 1) return 0;

    //convert to asci
    for(int i = 0; i < 256; i++) {
        char buf[3] = {0};
        sprintf(buf,"%02x", sig[i]);
        strcat(output,buf);
    }

    free(sig);
    printf("ASCI SIG:\n%s\n", output);

    //Verify
    unsigned char signature[256];
    char* pointer = output;
    //extract sig from hex asci
    for(int i = 0; i < 256; i++) {
        unsigned int value;
        sscanf(pointer, "%02x", &value);
        //printf("%02x", value);
        pointer = pointer + 2;
        signature[i] = value;
    }
    printf("\n");

    printf("size of key: %lu\n", strlen(pub_key) + 1);
    printf("%s\n", pub_key);

    BIO *bio = BIO_new_mem_buf((void*)pub_key, strlen(pub_key));
    RSA *rsa_pub = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);

    rc = RSA_verify(NID_sha256, data,32,signature,256,rsa_pub);
    if(rc != 1) printf("ERROR VERIFYING!\n"); else printf("VERIFIED!\n");



    return 1;
}

void post_music(char* input) {

    char note[3] = {0};

    sscanf(input, "%*s %s", note);
    

    char out_msg[2000] = {0};
    char seperator[] = " ";
    strcpy(out_msg, "M ");

    //Send Hex public key
    //char* start = pub_key + 31;
    char asci_pub_key[500] = {0};
    int i = 31;
    int x = 0;
    while (pub_key[i] != '-') {
        if(pub_key[i] != '\n') asci_pub_key[x++] = pub_key[i];
        i++;
    }

    printf("length: %lu\n", strlen(asci_pub_key));


    printf("PUBLIC KEY STRIPPED: %s\n", asci_pub_key);



    strcat(out_msg, asci_pub_key);
    strcat(out_msg, seperator);
    strcat(out_msg, note);
    char sig[513] = {0};
    message_signature(sig,out_msg,your_keys);
    strcat(out_msg, seperator);
    strcat(out_msg,sig);

    int sock_out = nn_socket (AF_SP, NN_PUSH);
    assert (sock_out >= 0);
    int timeout = 50;
    assert (nn_setsockopt(sock_out, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);
    sleep(1);

    printf("\nSending: %s\n", out_msg);
    strli_node* current = other_nodes->head;

    for(int i = 0; i < other_nodes->length; i++) {
        int eid = nn_connect (sock_out, current->value); 
        assert(eid >= 0);
        printf("Announcing to: %s, ", current->value);
        int bytes = nn_send (sock_out, out_msg, strlen(out_msg), 0);
        printf("Bytes sent: %d\n", bytes);
        current = current->next;
        nn_shutdown(sock_out,eid);
    }

    return;



}


void post_transaction(char* input) {

    //char sender[32];
    char recipient[32];
    char amount[32];

    //sscanf(input, "%*s %s %s %s", sender, recipient, amount);
    sscanf(input, "%*s %s %s", recipient, amount);


    //if(!val_trans_format(sender, recipient, amount))
    //    return;
    
    if(!val_trans_format(recipient, amount))
        return;

    char out_msg[2000] = {0};
    char seperator[] = " ";
    strcpy(out_msg, "T ");

    //Send Hex public key
    //char* start = pub_key + 31;
    char asci_pub_key[500] = {0};
    int i = 31;
    int x = 0;
    while (pub_key[i] != '-') {
        if(pub_key[i] != '\n') asci_pub_key[x++] = pub_key[i];
        i++;
    }

    printf("length: %lu\n", strlen(asci_pub_key));


    printf("PUBLIC KEY STRIPPED: %s\n", asci_pub_key);



    strcat(out_msg, asci_pub_key);
    strcat(out_msg, seperator);
    
    strcat(out_msg, recipient);
    strcat(out_msg, seperator);
    strcat(out_msg, amount);
    char sig[513] = {0};
    message_signature(sig,out_msg,your_keys);
    strcat(out_msg, seperator);
    strcat(out_msg,sig);

    c_transaction* temp = new_trans(0,asci_pub_key, recipient, atoi(amount), out_msg);
    add_to_queue(main_queue, temp);

    return;
}

//Read configuration file
int read_config() {
    FILE* config = fopen("node.cfg", "r");
    if(config == NULL) return 0;

    char buffer[120] = {0};
    while (fgets(buffer, sizeof(buffer), config)) {
        if(buffer[strlen(buffer) -1] == '\n') buffer[strlen(buffer) -1] = 0;
        strli_append(other_nodes, buffer);
    }
    fclose(config);

    return 0;
}


int create_keys() {

//Create keypair
    your_keys = RSA_generate_key(2048,3,NULL,NULL);

    //Create structures to seperate keys
    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    //Extract data out of RSA structure 
    PEM_write_bio_RSAPrivateKey(pri, your_keys, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, your_keys);

    //Get length of data
    size_t pri_len = BIO_pending(pri);
    size_t pub_len = BIO_pending(pub);

    //Prepare char buffers for keys
    pri_key = malloc(pri_len + 1);
    pub_key = malloc(pub_len + 1);

    //Read into buffers
    BIO_read(pri,pri_key,pri_len);
    BIO_read(pub, pub_key,pub_len);

    //Terminate strings
    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    return 1;

}



int main(void) {
    
    //Setup
    printf("Blockchain in C: Client v0.1 by DG\n'h' for help/commandlist\n");
    char buffer[120] = {0};
    main_queue = new_queue();
    other_nodes = create_strlist();
    read_config();

    //Create Keys
    create_keys();
    printf("%s%s", pri_key, pub_key);

    char buffer2[500];
    strcpy(buffer2, pub_key);


    //Network thread
    pthread_create(&network_thread, NULL, check_network, NULL);

    //Wait for command
    while(true) {
        printf("b-in-c>");
        fgets(buffer, 120, stdin);
        buffer[strlen(buffer) - 1] = 0;

        if( !strcmp("h",buffer) || !strcmp("help",buffer) )
            display_help();

        else if( !strcmp("q", buffer) || !strcmp("quit", buffer) || !strcmp("exit", buffer) )
            quit_program();

        else if( !strcmp("t", buffer) )
            queue_print(main_queue);

        else if(buffer[0] == 'n')
            post_transaction(buffer);
        else if(buffer[0] == 'p')
            post_music(buffer);
        else if( !strcmp("", buffer))
            ;
        else
            printf("Command not recognized.\n");

    }

    return 0;
}
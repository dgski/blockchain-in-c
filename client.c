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
#include "data_containers/linked_list.h"
#include "hash.h"

//IDs
RSA* your_keys;
char* pub_key;
char* pri_key;
char asci_pub_key[500] = {0};

transaction_queue*  main_queue;

//Threads
pthread_t inbound_network_thread;
pthread_t outbound_network_thread;
pthread_mutex_t our_mutex;
int close_threads;


//Lists
list* other_nodes;
list* outbound_msg_queue; //holds outbound message structs
list* inbound_msg_queue; //holds recieved char* messages to execute

//Sockets
int sock_in;
int sock_out;



//Message item structure
typedef struct message_item {
    char toWhom[300];
    char message[2000];
    unsigned int tries;
} message_item;

void setup_message(message_item* in_message) {
    
    in_message->tries = 0;
    memset(in_message->toWhom, 0, sizeof(in_message->toWhom));
    return;
}

void display_help() {
    printf("Help/Command List: 'h'\n");
    printf("Transactions List: 't'\n");
    printf("Submit a post: 'n sender reciever amount'\n");
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


    printf("MESSAGE: '%s'\n", message);


    //Hash the message
    unsigned char data[32];
    hash256(data,message);

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

//Send out post to everyone in other_nodes list
void* send_to_all(list* in_list, li_node* in_item, void* data) {
    
    if(in_item == NULL || in_item->size > 300) return NULL;

    //Get ip to send to
    char data_string[2000];
    memcpy(data_string,in_item->data,in_item->size);
    data_string[in_item->size] = 0;

    //Get message to send
    char* out_msg = (char*)data;

    if(strlen(out_msg) > 2000)
        return NULL;

   
    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom, data_string);
    strcpy(announcement.message, out_msg);

    pthread_mutex_lock(&our_mutex);
    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
    pthread_mutex_unlock(&our_mutex);

    return NULL;
}

void post(char* input) {

    char note[3] = {0};

    sscanf(input, "%*s %s", note);
    
    char out_msg[2000] = {0};
    char seperator[] = " ";
    strcpy(out_msg, "P ");

    strcat(out_msg, asci_pub_key);
    strcat(out_msg, seperator);
    strcat(out_msg, note);
    char sig[513] = {0};
    message_signature(sig,out_msg + 2,your_keys);
    strcat(out_msg, seperator);
    strcat(out_msg,sig);
    
    li_foreach(other_nodes,send_to_all, &out_msg);

    return;

}


void post_transaction(char* input) {

    char recipient[32];
    char amount[32];

    sscanf(input, "%*s %s %s", recipient, amount);

    int the_amount = atoi(amount);
    sprintf(amount,"%010d", the_amount);


    //if(!val_trans_format(sender, recipient, amount))
    //    return;
    
    if(!val_trans_format(recipient, amount))
        return;

    char out_msg[2000] = {0};
    char seperator[] = " ";
    strcpy(out_msg, "T ");

    strcat(out_msg, asci_pub_key);
    strcat(out_msg, seperator);
    
    strcat(out_msg, recipient);
    strcat(out_msg, seperator);
    strcat(out_msg, amount);
    char sig[513] = {0};
    message_signature(sig,out_msg + 2,your_keys);
    strcat(out_msg, seperator);
    strcat(out_msg,sig);

    li_foreach(other_nodes,send_to_all, &out_msg);


    /*
    c_transaction* temp = new_trans(0,asci_pub_key, recipient, atoi(amount), out_msg);
    add_to_queue(main_queue, temp);*/

    return;
}

//Read configuration file
int read_config2() {
    FILE* config = fopen("node.cfg", "r");
    if(config == NULL) return 0;

    char buffer[120] = {0};
    while (fgets(buffer, sizeof(buffer), config)) {
        if(buffer[strlen(buffer) -1] == '\n') buffer[strlen(buffer) -1] = 0;
        li_append(other_nodes, buffer, strlen(buffer) + 1);
    }
    fclose(config);

    return 0;
}

//Create keypair
int create_keys() {
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

    int i = 31;
    int x = 0;
    while (pub_key[i] != '-') {
        if(pub_key[i] != '\n') asci_pub_key[x++] = pub_key[i];
        i++;
    }

    printf("length: %lu\n", strlen(asci_pub_key));


    printf("PUBLIC KEY STRIPPED: %s\n", asci_pub_key);

    return 1;

}


//Done to each message in 'outbound_msg_queue'. input is of type message_item struct
void* process_outbound(list* in_list, li_node* input, void* data) {

    if(input == NULL) return NULL;

    sock_out = nn_socket(AF_SP, NN_PUSH);
    assert (sock_out >= 0);
    int timeout = 100;
    assert (nn_setsockopt(sock_out, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);

    message_item* our_message = (message_item*)input->data;

    printf("Sending to: %s, ",our_message->toWhom);
    if(nn_connect (sock_out, our_message->toWhom) < 0){
        printf("Connection Error.\n");
        nn_close(sock_out);
    }
    int bytes = nn_send (sock_out, our_message->message, strlen(our_message->message), 0);
    printf("Bytes sent: %d\n", bytes);
    usleep(100000);
    nn_close(sock_out);

    if(bytes > 0 || our_message->tries == 0) li_delete_node(in_list, input);
    else our_message->tries++;

    return NULL;
}


//Outbound thread function - tried to send everything in outbound message queue
void* out_server() {
    while(true) {
        pthread_mutex_lock(&our_mutex);
        li_foreach(outbound_msg_queue, process_outbound, NULL);
        if(close_threads)
            return NULL;
        pthread_mutex_unlock(&our_mutex);

        sleep(1);
    }
}



int main(void) {
    
    //Setup
    printf("Blockchain in C: Client v0.1 by DG\n'h' for help/commandlist\n");
    char buffer[120] = {0};
    main_queue = new_queue();
    other_nodes = list_create();
    outbound_msg_queue = list_create();
    inbound_msg_queue = list_create();

    //read_config();
    read_config2();

    //Create Keys
    create_keys();
    printf("%s%s", pri_key, pub_key);

    char buffer2[500];
    strcpy(buffer2, pub_key);


    //Threads
    pthread_mutex_t our_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_create(&outbound_network_thread, NULL, out_server, NULL);
    close_threads = 0;

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
            post(buffer);
        else if( !strcmp("", buffer))
            ;
        else
            printf("Command not recognized.\n");

    }

    return 0;
}
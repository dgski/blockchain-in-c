#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <signal.h>


#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"

#include "blockchain.h"
#include "linked_list_string.h"
#include "hash2.h"

#define DEBUG 0

//Global variables
char node_name[60];
unsigned int node_earnings;
int* beaten;
blockchain* our_chain;

blockchain* foreign_chain;
int expected_length;

pthread_t network_thread;
char our_ip[100] = {0};
strlist* other_nodes;

//Send out current chain length
void* announce_length(strlist* in_list, strli_node* in_item) {

    if(!strcmp(in_item->value, our_ip)) return NULL;

    int sock = nn_socket(AF_SP, NN_PUSH);

    assert (sock >= 0);
    int timeout = 50;
    assert (nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);
    assert (nn_connect (sock, in_item->value) >= 0);

    printf("Announcing chain length to: %s, ", in_item->value);
    char message[2000];
    strcpy(message, "L ");
    char length_buffer[21];
    sprintf(length_buffer,"%d",our_chain->length);
    strcat(message, length_buffer);
    strcat(message, " ");
    strcat(message, our_ip);
    int bytes = nn_send (sock, message, strlen(message), 0);
    printf("Bytes sent: %d\n", bytes);
    nn_shutdown(sock,0);


    return NULL;
}


//Send new block block to other nodes
void* announce_block(strlist* in_list, strli_node* in_item) {

    if(!strcmp(in_item->value, our_ip)) return NULL;

    int sock = nn_socket(AF_SP, NN_PUSH);
    assert (sock >= 0);
    int timeout = 50;
    assert (nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);
    assert (nn_connect (sock, in_item->value) >= 0);

    printf("Announcing to: %s, ", in_item->value);
    char message[2000];
    strcpy(message, "B> ");
    strcat(message, our_chain->last_block);
    int bytes = nn_send (sock, message, strlen(message), 0);
    printf("Bytes sent: %d\n", bytes);
    nn_shutdown(sock,0);

    return NULL;
}

void* announce_existance(strlist* in_list, strli_node* in_item) {

    if(!strcmp(in_item->value, our_ip)) return NULL;

    int sock = nn_socket(AF_SP, NN_PUSH);
    assert (sock >= 0);
    int timeout = 50;
    assert (nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);
    assert (nn_connect (sock, in_item->value) >= 0);
    printf("Announcing existance to: %s, ", in_item->value);
    char message[2000];
    strcpy(message, "N ");
    strcat(message, our_ip);
    int bytes = nn_send (sock, message, strlen(message), 0);
    printf("Bytes sent: %d\n", bytes);
    nn_shutdown(sock,0);

    return NULL;
}


//Continually searches for proper proof of work
int mine() {
    sleep(1);
    printf("Mining started.\n");
    long result;
    
    while(true) {

        if(DEBUG) {
            char buffer[120] = {0};
            fgets(buffer, sizeof(buffer), stdin);
        }
        
        unsigned int time_1 = time(NULL);
        result = proof_of_work(beaten, our_chain->last_hash);
        unsigned int time_2 = time(NULL);

        if(result > 0) {

            //printf("PROOF OF WORK FOUND: %d\n", result);
            printf(ANSI_COLOR_GREEN);

            printf("\nMINED: %.4f min(s)\n", (time_2 - time_1)/60.0);

            our_chain->last_proof_of_work = result;
            new_transaction(our_chain,node_name,node_name, 2, "hello");
            node_earnings += 2;
            blink* a_block = append_current_block(our_chain, our_chain->last_proof_of_work);
            print_block(a_block,'-');

            printf(ANSI_COLOR_RESET);
            //char block_buffer[BLOCK_STR_SIZE];
            //string_block(block_buffer, &a_block->data);
            //printf("%s\n", block_buffer);
            //strli_foreach(other_nodes,announce_block); //Block announcement
            strli_foreach(other_nodes,announce_length);
            }
        else {
            printf("Abandoning our in-progress block.\n");
            *beaten = 0;
        }
        printf("\nTotal Node Earnings: %d noins\n", node_earnings);

    }
}


bool verify_transaction(const char* input, char* sender, char* recipient, char* amount, char* signature) {
    
    char data[1500] = {0};

    strcat(data, sender);
    strcat(data, " ");

    strcat(data, recipient);
    strcat(data," ");

    //char buffer[20] = {0};
    //sprintf(buffer, "%d", amount);
    strcat(data, amount);

    //printf("MESSAGE: %s\n", data);

    unsigned char hash_value[32];
    aahash256(hash_value,data);
    /*printf("HASHVALUE:\n");
    for(int i= 0; i < 32; i++)
        printf("%02x", hash_value[i]);
    printf("\n");*/

    unsigned char sig[256];
    char* pointer = signature;
    //extract sig from hex asci
    for(int i = 0; i < 256; i++) {
        unsigned int value;
        sscanf(pointer, "%02x", &value);
        pointer = pointer + 2;
        sig[i] = value;
    }
    //printf("\n");

    //printf("\n\nASCI SIG:\n%s\n\n", signature);

    char our_key[1000] = {0};
    char* new_key_point = our_key;
    char* send_pointer = sender;

    for(int i = 0; i < 5; i++) {
        memcpy(new_key_point,send_pointer,64);
        new_key_point = new_key_point + 64;
        send_pointer = send_pointer + 64;
        *new_key_point++ = '\n';
    }

    memcpy(new_key_point,send_pointer,41);

    char final_key[427] = {0};
    sprintf(final_key,"-----BEGIN RSA PUBLIC KEY-----\n%s\n-----END RSA PUBLIC KEY-----\n", our_key);

    //printf("%s", final_key);




    char* pub_key = final_key;

    //printf("size of key: %lu\n", strlen(pub_key) + 1);
    //printf("\n%s\n", pub_key);

    BIO *bio = BIO_new_mem_buf((void*)pub_key, strlen(pub_key));
    RSA *rsa_pub = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);

    int rc = RSA_verify(NID_sha256, hash_value,32,sig,256,rsa_pub);
    //printf("VERIFY RETURN: %d\n", rc);
    if(rc != 1) printf("Invalid."); else printf("Valid.");

    if(rc) return true; else return false;
}


//Insert transaction [sender receiver amount]
int insert_trans(char* input) {

    printf("\nVerifying Transaction...");

    char* sender = strtok(input," ");
    char* recipient = strtok(NULL, " ");
    char* amount = strtok(NULL, " ");
    char* signature = strtok(NULL, " ");

    //Check if user has enough in quick ledger
    //Checking quickledger....

    //Check if transaction is signed
    if(!verify_transaction(input,sender, recipient, amount, signature))
        return -1;

    printf("\nInserting.\n");

    int amount2;
    sscanf(amount,"%d",&amount2);

    new_transaction(our_chain,sender,recipient,amount2,signature);

    return 0;
}

//Insert music [sender music]
int insert_post(const char* input) {

    printf("Inserting Music!\n");
    char sender[32] = {0};
    char music[64] = {0};
    sscanf(input, "%s %s", sender, music);
    return 0;
}
//Verify Foreign Block
void verify_foreign_block(const char* input) {
    //printf("Verifying Foreign Block: %s\n", input);
    printf("Verifying Foreign Block... ");

    char f_block[1000] = {0};
    strcpy(f_block, input);

    char* index = strtok(f_block,".");
    char* time = strtok(NULL, ".");
    char* transactions = strtok(NULL, ".");
    char* trans_size = strtok(NULL, ".");
    char* proof = strtok(NULL, ".");
    char* hash = strtok(NULL, ".");
    
    long the_proof;
    sscanf(proof,"%020ld",&the_proof);

    //Check if transactions are valid

    if(valid_proof(our_chain->last_hash, the_proof)) {
        printf("Valid.\n");



        char posts[] = {};
        transaction rec_trans[20] = {0};
        extract_transactions(rec_trans, transactions);
        //Add block and reset chain
        blink* a_block = append_new_block(our_chain, atoi(index), atoi(time),rec_trans, posts, atoi(trans_size), the_proof);
        printf(ANSI_COLOR_CYAN);
        printf("\nABSORBED:\n");
        print_block(a_block,'-');
        printf(ANSI_COLOR_RESET);

        //Add received block

        *beaten = 1; //Will stop mining current block
    }
    else
        printf("Invalid.\n");


    return;
}

//Regster New Node [node_ip]
void register_new_node(char* input) {
    
    printf("Registering New Node...");
    if(strli_search(other_nodes, NULL, input)) {
        printf(" Already registered.");
        return;
    }

    strli_append(other_nodes, input);
    printf("Added '%s'\n", input);

    //Anounce your chain length
    int sock = nn_socket(AF_SP, NN_PUSH);
    assert (sock >= 0);
    int timeout = 50;
    assert (nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);
    assert (nn_connect (sock, input) >= 0);
    sleep(1);

    printf("Sending Chain Length to: %s, ", input);
    char message[2000];
    strcpy(message, "L ");
    char length_buffer[21];
    sprintf(length_buffer,"%d",our_chain->length);
    strcat(message, length_buffer);
    strcat(message, " ");
    strcat(message, our_ip);
    int bytes = nn_send (sock, message, strlen(message), 0);
    printf("Bytes sent: %d\n", bytes);
    nn_shutdown(sock,0);


}

int request_chain(char* address) {

    //Request chain foreign chain
    int sock = nn_socket(AF_SP, NN_PUSH);
    assert (sock >= 0);
    int timeout = 50;
    assert (nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);
    assert (nn_connect (sock, address) >= 0);
    sleep(1);

    printf("Requesting chain from: %s, ", address);
    char message[2000];
    strcpy(message, "C ");
    strcat(message, our_ip);
    int bytes = nn_send (sock, message, strlen(message), 0);
    printf("Bytes sent: %d\n", bytes);
    nn_shutdown(sock,0);

    return 0;

}

int compare_length(char* input) {

    printf("Checking foreign chain length... ");
    unsigned int foreign_length;
    char Whom[200];
    sscanf(input, "%d %s", &foreign_length, Whom);

    if(foreign_length > our_chain->length || true) {
        printf("Someone claims they are a winner!\n");
        expected_length = foreign_length;
        request_chain(Whom);
    }
    else
        printf("False alarm. Our chain is still good!\n");

    return 0;
}

int send_chain(char* address) {

    int sock = nn_socket(AF_SP, NN_PUSH);
    assert (sock >= 0);
    int timeout = 50;
    assert (nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);
    assert (nn_connect (sock, address) >= 0);
    printf("Sending blockchain existance to: %s, ", address);
    char message[2200] = {0};

    blink* temp = our_chain->head;
    for(int i = 0; i < our_chain->length + 1; i++) {

        strcpy(message, "B+ ");
        char block[2000] = {0};
        string_block(block,&temp->data);
        strcat(message, block);
        
        int bytes = nn_send (sock, message, strlen(message), 0);
        printf("Bytes sent: %d\n", bytes);

        temp = temp->next;
    }

    nn_shutdown(sock,0);

    return 0;
}

int chain_incoming(char* input) {

    char f_block[2000] = {0};
    strcpy(f_block, input);

    char* index = strtok(f_block,".");
    char* time = strtok(NULL, ".");
    char* transactions = strtok(NULL, ".");
    char* trans_size = strtok(NULL, ".");
    char* proof = strtok(NULL, ".");
    char* hash = strtok(NULL, ".");

    printf("Block with index %d recieved\n", atoi(index));

     if(atoi(index) == 0) {
        foreign_chain = new_chain();
        printf("Created genesis block for foreign entity.\n");
        return 0;
    }


    long the_proof;
    sscanf(proof,"%020ld",&the_proof);
    
    
    if(valid_proof(foreign_chain->last_hash, the_proof)){
        printf("Valid.\n");

        char posts[] = {};
        transaction rec_trans[20] = {0};
        extract_transactions(rec_trans, transactions);
        //Add block and reset chain
        blink* a_block = append_new_block(foreign_chain, atoi(index), atoi(time),rec_trans, posts, atoi(trans_size), the_proof);
        printf(ANSI_COLOR_CYAN);
        //printf("\nVerified Foreign Block with index %d.\n", atoi(index));
        //print_block(a_block,'-');
        printf(ANSI_COLOR_RESET);

        if((atoi(index)) > our_chain->length && (atoi(index) == expected_length)) {

            printf("\nComplete chain is verified and longer. Make that ours.\n");

            printf(ANSI_COLOR_CYAN);
            printf("\nABSORBED TOP BLOCK:\n");
            print_block(a_block,'-');
            printf(ANSI_COLOR_RESET);


            discard_chain(our_chain);
            our_chain = foreign_chain;
            foreign_chain = NULL;
            
            *beaten = 1;

        }
    }



    return 0;
}  


//Message type: T: transaction, P: post, B: block, N: new node, L: blockchain length, C: chain
void process_message(const char* in_msg) {

    char to_process[1000] = {0};
    strcpy(to_process, in_msg);

    char* token = strtok(to_process," ");
    //printf("MESSAGE TYPE: %s\n", token);

    //printf("MESSAGE CONTENTS: %s\n", in_msg + 2);

    if(!strcmp(token, "T"))
        insert_trans(to_process + 2);
    if(!strcmp(token, "P"))
        insert_post(to_process + 2);
    if(!strcmp(token, "B>"))
        verify_foreign_block(to_process + 3);
    if(!strcmp(token, "B+"))
        chain_incoming(to_process + 3);
    if(!strcmp(token, "N"))
        register_new_node(to_process + 2);
    if(!strcmp(token, "L"))
        compare_length(to_process + 2);
    if(!strcmp(token, "C"))
        send_chain(to_process + 2);


}
void* send_node_msg(strlist* in_list, strli_node* item) {

    printf("Sending: %s\n", item->value);
    char buffer[2000] = {0};
    strcpy(buffer, item->value);
    char* toWhom = strtok(buffer,"#");
    char* out_msg = strtok(NULL, "#");

    printf("toWHom: %s\n", toWhom);
    printf("out_msg: %s\n", out_msg);

    if("message sent")
        return (void*)1;
    else
        return (void*)0;

    return NULL;
}

//Network interface function
void* server() {
    
    printf("Blockchain in C Major: Server v0.1\n");
    printf("Node name: %s\n\n", node_name);

    int sock_in = nn_socket (AF_SP, NN_PULL);
    assert (sock_in >= 0);
    assert (nn_bind (sock_in, our_ip) >= 0);
    int timeout = 200;
    assert (nn_setsockopt (sock_in, NN_PULL, NN_RCVTIMEO, &timeout, sizeof (timeout)));
    printf("Socket Ready!\n");

    char buf[1000] = {0};

    while(true) {
        
        //Receive
        memset(buf, 0, sizeof(buf));
        int bytes = nn_recv(sock_in, buf, sizeof(buf), 0);
        if(bytes > 0) {
            printf("\nRecieved: \"%s\"\n", buf);
            process_message(buf);
        }

        //Send
        //sleep(1);
        //printf("sending out...\n");
        //strli_foreach(outbound_msgs, send_node_msg);

    }
    

    return 0;
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

void graceful_shutdown(int dummy) {
    printf("\nCommencing graceful shutdown!\n");
    
    strli_discard(other_nodes);
    free(beaten);
    exit(0);
}

int send_out(char* address, char* message) {

    return 1;

}



//Main function
int main(int argc, char* argv[]) {
    
    //Ctrl-C Handler
    signal(SIGINT, graceful_shutdown);

    //Create blockchain
    our_chain = new_chain();

    //Beaten variable
    beaten = malloc(sizeof(int));
    *beaten = 0;

    //Create list of other nodes
    other_nodes = create_strlist();
    read_config();


    //Generate random node name
    srand(time(NULL));   // should only be called once
    int r = rand();   
    sprintf(node_name, "node%d", r);

    //Get our ip address from argv
    if(argc < 2)
        strcpy(our_ip, "ipc:///tmp/pipeline_0.ipc");
    else
        strcpy(our_ip, argv[1]);
    
    //Send out our existence
    strli_foreach(other_nodes,announce_existance);



    //Reset node earnings
    node_earnings = 0;

    //Listen and respond to network connections
    pthread_create(&network_thread, NULL, server, NULL);

    //Begin mining
    mine();

    return 0;

}
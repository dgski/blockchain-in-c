#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"

#include "blockchain.h"
#include "data_containers/linked_list.h"
#include "node.h"

#define DEBUG 1
#define MESSAGE_LENGTH 2000

/////////////////////////////////////////////////////
//GLOBAL VARIABLES
/////////////////////////////////////////////////////

//Identification
char node_name[60];
char our_ip[120] = {0};

//Blockchains
blockchain* our_chain;
blockchain* foreign_chain;
dict* foreign_chains;

unsigned int node_earnings;
int expected_length;
int* beaten;

//Threads
pthread_t inbound_network_thread;
pthread_t outbound_network_thread;
pthread_t inbound_executor_thread;
pthread_mutex_t our_mutex;

//Lists
list* other_nodes;
list* outbound_msg_queue; //holds outbound message structs
list* inbound_msg_queue; //holds recieved char* messages to execute


//Sockets
int sock_in;
int sock_out;



/////////////////////////////////////////////////////
//FUNCTIONS
/////////////////////////////////////////////////////
int print_balance(bt_node* current_node) {
    
    int* balance = current_node->data;
    printf("%s : %d\n", current_node->key, *balance);
    return 1;
}

int print_keys(bt_node* current_node) {
    
    printf("%s,", current_node->key);
    return 1;
}




//Continually search for proper proof of work
int mine() {
    sleep(3);
    printf("\nMining started.\n");
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
            printf(ANSI_COLOR_GREEN);
            printf("\nMINED: %.4f min(s)\n", (time_2 - time_1)/60.0);

            our_chain->last_proof_of_work = result;
            new_transaction(our_chain,node_name,node_name, 2, "hello");
            node_earnings += 2;
            blink* a_block = append_current_block(our_chain, our_chain->last_proof_of_work);
            print_block(a_block,'-');

            printf(ANSI_COLOR_RESET);
            li_foreach(other_nodes,announce_length, NULL);

            }
        else {
            printf("Abandoning our in-progress block.\n");
            *beaten = 0;
        }
        int our_earnings = 0;
        void* our_ledger_earnings = dict_access(our_chain->quickledger,node_name);
        if(our_ledger_earnings != NULL)
            our_earnings += *((int*)our_ledger_earnings);


        printf("\nQUICKLEDGER BALANCES:\n");
            dict_foreach(our_chain->quickledger, print_balance, NULL);
        
        printf("\nTotal Node Earnings: %d noins\n", our_earnings);

    }
}

//Send out current chain's length to everyone
void* announce_length(list* in_list, li_node* in_item, void* data) {

    char* data_string = malloc(in_item->size);
    memcpy(data_string,in_item->data,in_item->size);

    if(!strcmp(data_string, our_ip)) {
        free(data_string);
        return NULL;
    }

    char message[2000];
    strcpy(message, "L ");
    char length_buffer[21];
    sprintf(length_buffer,"%d",our_chain->length);
    strcat(message, length_buffer);
    strcat(message, " ");
    strcat(message, our_ip);

    message_item chain_send;
    setup_message(&chain_send);
    strcpy(chain_send.toWhom, data_string);
    strcpy(chain_send.message, message);

    pthread_mutex_lock(&our_mutex);
    li_append(outbound_msg_queue,&chain_send, sizeof(chain_send));
    pthread_mutex_unlock(&our_mutex);

    free(data_string);

    return NULL;
}

//Send out own ip address to everyone in other_nodes list
void* announce_existance(list* in_list, li_node* in_item, void* data) {
    
    char* data_string = malloc(in_item->size);
    memcpy(data_string,in_item->data,in_item->size);

    if(!strcmp(data_string, our_ip)){
        free(data_string);
        return NULL;
    }
   
    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom, data_string);
    strcpy(announcement.message, "N ");
    strcat(announcement.message, our_ip);

    pthread_mutex_lock(&our_mutex);
    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
    pthread_mutex_unlock(&our_mutex);

    free(data_string);

    return NULL;
}

//Insert transaction [sender receiver amount]
int insert_trans(char* input) {

    printf("\nVerifying Transaction...");

    char* sender = strtok(input," ");
    char* recipient = strtok(NULL, " ");
    char* amount = strtok(NULL, " ");
    char* signature = strtok(NULL, " ");

    //Check if user has enough in quick ledger
    void* sender_balance = dict_access(our_chain->quickledger, sender);

    if(sender_balance == NULL || (int)sender_balance < atoi(amount)) {
        printf("Insufficient Funds.\n");
        return 0;
    }




    //Check if transaction is signed
    if(!verify_signiture(input,sender, recipient, amount, signature))
        return 0;

    printf("\nInserting.\n");

    int amount2;
    sscanf(amount,"%d",&amount2);

    new_transaction(our_chain,sender,recipient,amount2,signature);

    return 1;
}

//Insert [post] [sender post]
int insert_post(const char* input) {

    printf("Inserting Music!\n");
    char sender[32] = {0};
    char music[64] = {0};
    sscanf(input, "%s %s", sender, music);
    return 0;
}

//Regster New Node and send out your chain length
void register_new_node(char* input) {
    
    printf("Registering New Node...");
    if(li_search(other_nodes,NULL,input,strlen(input) + 1)) {
        printf(" Already registered. ");
    }
    else {
        li_append(other_nodes, input, strlen(input) + 1);
        printf("Added '%s'\n", input);
    }

    printf("Sending chain length to: %s\n ", input);
    char message[2000];
    strcpy(message, "L ");
    char length_buffer[21];
    sprintf(length_buffer,"%d",our_chain->length);
    strcat(message, length_buffer);
    strcat(message, " ");
    strcat(message, our_ip);

    message_item chain_send;
    setup_message(&chain_send);
    strcpy(chain_send.toWhom, input);
    strcpy(chain_send.message, message);
    
    li_append(outbound_msg_queue,&chain_send, sizeof(chain_send));

}

//Send request for complete chain to address of claimant
int request_chain(char* address) {

    printf("Requesting chain from: %s, ", address);
    char message[MESSAGE_LENGTH];
    strcpy(message, "C ");
    strcat(message, our_ip);

    message_item request_chain;
    setup_message(&request_chain);
    strcpy(request_chain.toWhom, address);
    strcpy(request_chain.message, message);
    
    li_append(outbound_msg_queue,&request_chain, sizeof(request_chain));

    return 0;

}

//Compare received chain length to our length, if bigger request more
int compare_length(char* input) {

    printf("Checking foreign chain length... ");
    unsigned int foreign_length;
    char Whom[200];
    sscanf(input, "%d %s", &foreign_length, Whom);

    if(foreign_length > our_chain->length) {
        printf("Someone claims they are a winner!\n");
        expected_length = foreign_length;
        request_chain(Whom);
    }
    else
        printf("False alarm. Our chain is still good!\n");

    return 0;
}

//Send out our complete chain as block messages
int send_our_chain(char* address) {

    if(strlen(address) > 120) return 0;
    printf("Sending blockchain to: %s, ", address);
    char message[MESSAGE_LENGTH];

    if(our_chain->head == NULL) return 0;
    blink* temp = our_chain->head;

    for(int i = 0; i < our_chain->length + 1; i++) {

        strcpy(message, "B+ ");
        char block[2000] = {0};
        string_block(block,&temp->data);
        strcat(message, block);

        message_item the_chain;
        setup_message(&the_chain);
        strcpy(the_chain.toWhom, address);
        strcpy(the_chain.message, message);
        
        li_append(outbound_msg_queue,&the_chain, sizeof(the_chain));

        temp = temp->next;
    }

    return 1;
}

//Send out our complete chain as block messages
int send_our_chain2(char* address) {

    if(strlen(address) > 120) return 0;
    printf("Sending blockchain to: %s, ", address);
    char message[MESSAGE_LENGTH];

    if(our_chain->head == NULL) return 0;
    blink* temp = our_chain->head;

    char rand_chain_id[60];
    int r = rand();   
    sprintf(rand_chain_id, "CHIN%010d.", r);

    for(int i = 0; i < our_chain->length + 1; i++) {

        strcpy(message, "B+ ");
        char block[2000] = {0};
        strcat(message, rand_chain_id);
        char the_length[12];
        sprintf(the_length,"%010d.", our_chain->length);
        strcat(message, the_length);
        string_block(block,&temp->data);
        strcat(message, block);

        message_item the_chain;
        setup_message(&the_chain);
        strcpy(the_chain.toWhom, address);
        strcpy(the_chain.message, message);
        
        li_append(outbound_msg_queue,&the_chain, sizeof(the_chain));

        temp = temp->next;
    }

    return 1;
}

int verify_foreign_block(char* input) {

    char f_block[MESSAGE_LENGTH] = {0};
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

    if(foreign_chain == NULL) return 0;

    printf("Index Recieved: %d\n",atoi(index));
    printf("Expected length: %d\n",expected_length);
    printf("Our length: %d\n",our_chain->length);
    printf("Last Hash Length: %lu\n", strlen(foreign_chain->last_hash));
    printf("Last Hash: %s\n",foreign_chain->last_hash);

    if(valid_proof(foreign_chain->last_hash, the_proof)){
        printf("Valid.\n");

        char posts[] = {};
        transaction rec_trans[20] = {0};
        extract_transactions(foreign_chain, rec_trans, transactions);
        
        //Add block
        blink* a_block = append_new_block(foreign_chain, atoi(index), atoi(time),rec_trans, posts, atoi(trans_size), the_proof);

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
    else {
        printf("Invalid.\n");
    }

    return 0;
}

int verify_foreign_block2(char* input) {

    char f_block[MESSAGE_LENGTH] = {0};
    strcpy(f_block, input);
    
    char* chain_id = strtok(f_block,".");
    char* exp_length = strtok(NULL, ".");
    char* index = strtok(NULL, ".");
    char* time = strtok(NULL, ".");
    char* transactions = strtok(NULL, ".");
    char* trans_size = strtok(NULL, ".");
    char* proof = strtok(NULL, ".");
    char* hash = strtok(NULL, ".");

    printf("Block of %s with index %d recieved\n", chain_id, atoi(index));

    alt_chain* this_chain = dict_access(foreign_chains,chain_id);

    if(this_chain == NULL && atoi(index) == 0) {

        printf("Creating chain for this block.\n");

        this_chain = malloc(sizeof(this_chain));
        this_chain->expected_length = atoi(exp_length);
        this_chain->expected_index = 1;
        this_chain->the_chain = malloc(sizeof(blockchain));
        this_chain->the_chain = new_chain();

        dict_insert(foreign_chains,chain_id,this_chain,sizeof(alt_chain));

        free(this_chain);
        return 1;
    }
    
    else if(this_chain == NULL && atoi(index) != 0) {
        printf("No chain for non-zero index block.\n");
        return 0;
    }

    if(atoi(index) != this_chain->expected_index) {
        printf("Not expecting this index number\n");
        return 0;
    }
    else {
        this_chain->expected_index++;
    }

    long the_proof;
    sscanf(proof,"%020ld",&the_proof);

    printf("Index Recieved: %d\n",atoi(index));
    printf("Expected length: %d\n",this_chain->expected_length);
    printf("Our length: %d\n",our_chain->length);
    printf("Last Hash Length: %lu\n", strlen(this_chain->the_chain->last_hash));
    printf("Last Hash: %s\n",this_chain->the_chain->last_hash);

    if(valid_proof(this_chain->the_chain->last_hash, the_proof)){
        printf("Valid.\n");

        char posts[] = {};
        transaction rec_trans[20] = {0};
        extract_transactions(this_chain->the_chain, rec_trans, transactions);
        
        //Add block
        blink* a_block = append_new_block(this_chain->the_chain, atoi(index), atoi(time),rec_trans, posts, atoi(trans_size), the_proof);

             printf("\nFOREIGN DICT SIZE %d:\n", foreign_chains->size);
            dict_foreach(foreign_chains, print_keys,NULL);
            printf("\n");


        if((atoi(index) == this_chain->expected_length) ) {
            
            if(atoi(index) > our_chain->length) {
                printf("\nComplete chain is verified and longer. Make that ours.\n");

                printf(ANSI_COLOR_CYAN);
                printf("\nABSORBED TOP BLOCK:\n");
                print_block(a_block,'-');
                printf(ANSI_COLOR_RESET);

                discard_chain(our_chain);
                our_chain = this_chain->the_chain;
                dict_del_elem(foreign_chains,chain_id,1);
            
                *beaten = 1;
            }
            else {
                printf("Received end of chain. Not longer. Throwing away.\n");
                dict_del_elem(foreign_chains,chain_id,0);
            }

            printf("\nFOREIGN DICT SIZE %d:\n", foreign_chains->size);
            dict_foreach(foreign_chains, print_keys,NULL);
            printf("\n");

        }
    }
    else {
        printf("Invalid.\n");
    }

    return 0;
}  


//Message type: T: transaction, P: post, B: block, N: new node, L: blockchain length, C: chain
void process_message(const char* in_msg) {

    char to_process[MESSAGE_LENGTH] = {0};
    strcpy(to_process, in_msg);

    char* token = strtok(to_process," ");
    //printf("MESSAGE TYPE: %s\n", token);

    //printf("MESSAGE CONTENTS: %s\n", in_msg + 2);

    if(!strcmp(token, "T"))
        insert_trans(to_process + 2);
    if(!strcmp(token, "P"))
        insert_post(to_process + 2);
    if(!strcmp(token, "B>"))
        /*verify_foreign_block(to_process + 3);*/ verify_foreign_block2(to_process + 3);
    if(!strcmp(token, "B+"))
        /*verify_foreign_block(to_process + 3);*/ verify_foreign_block2(to_process + 3);
    if(!strcmp(token, "N"))
        register_new_node(to_process + 2);
    if(!strcmp(token, "L"))
        compare_length(to_process + 2);
    if(!strcmp(token, "C"))
        /*send_our_chain(to_process + 2);*/ send_our_chain2(to_process + 2);

}

//Inbound thread function - receives messages and adds them to execution queue
void* in_server() {
    
    printf("Blockchain in C Major: Server v0.1\n");
    printf("Node name: %s\n\n", node_name);

    sock_in = nn_socket (AF_SP, NN_PULL);
    assert (sock_in >= 0);
    assert (nn_bind (sock_in, our_ip) >= 0);
    //int timeout = 500;
    //assert (nn_setsockopt (sock_in, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof (timeout)) >= 0);

    printf("Inbound Socket Ready!\n");

    char buf[MESSAGE_LENGTH];

    while(true) {
        usleep(10000);
        int bytes = nn_recv(sock_in, buf, sizeof(buf), 0);
        if(bytes > 0) {
            buf[bytes] = 0;
            printf("\nRecieved %d bytes: \"%s\"\n", bytes, buf);

            pthread_mutex_lock(&our_mutex);
            li_append(inbound_msg_queue,buf,bytes);
            pthread_mutex_unlock(&our_mutex);
        }
    }
    return 0;
}

//Executes everything in execution queue
void* inbound_executor() {
    while(true) {
        sleep(1);
        pthread_mutex_lock(&our_mutex);
        li_foreach(inbound_msg_queue, process_inbound, NULL);
        pthread_mutex_unlock(&our_mutex);
    }
}

//Executed the message, input is of type message_item struct
void* process_inbound(list* in_list, li_node* input, void* data) {

    if(input == NULL) return NULL;

    char* the_message = malloc(input->size + 1);
    memset(the_message, 0, input->size + 1);
    memcpy(the_message,input->data,input->size);
    process_message(the_message);
    free(the_message);

    li_delete_node(in_list, input);

    return NULL;
}

//Outbound thread function - tried to send everything in outbound message queue
void* out_server() {
    while(true) {
        pthread_mutex_lock(&our_mutex);
        li_foreach(outbound_msg_queue, process_outbound, NULL);
        pthread_mutex_unlock(&our_mutex);
        sleep(1);
    }
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

//Print outbound queue
void* print_queue(void* input) {

    if(input == NULL) return NULL;

    message_item* item = (message_item*)input;
    printf("- toWhom: %s, ", item->toWhom);
    printf("Message: %s\n", item->message);
    return NULL;

}

//Read configuration file
int read_config() {
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

//Free all outstanding memory
void graceful_shutdown(int dummy) {
    printf("\nCommencing graceful shutdown!\n");

    //Discard lists
    li_discard(other_nodes);
    li_discard(outbound_msg_queue);
    li_discard(inbound_msg_queue);

    //Discard blockchains
    discard_chain(our_chain);
    discard_chain(foreign_chain);
    
    pthread_mutex_destroy(&our_mutex);

    nn_close(sock_in);
    nn_close(sock_out);
    free(beaten);
    exit(0);
}

//Empty message
void setup_message(message_item* in_message) {
    
    in_message->tries = 0;
    memset(in_message->toWhom, 0, sizeof(in_message->toWhom));
    return;
}

//The star of the show
int main(int argc, char* argv[]) {
    
    //Ctrl-C Handler
    signal(SIGINT, graceful_shutdown);

    //Create our blockchain
    our_chain = new_chain();
    beaten = malloc(sizeof(int));
    *beaten = 0;

    //Create foreign chain dict
    foreign_chains = dict_create();

    //Create list of other nodes
    other_nodes = list_create();
    read_config();

    //Generate random node name
    srand(time(NULL));   // should only be called once
    int r = rand();   
    sprintf(node_name, "node%d", r);

    //Get our ip address from argument
    if(argc < 2)
        strcpy(our_ip, "ipc:///tmp/pipeline_0.ipc");
    else
        strcpy(our_ip, argv[1]);

    //Create list of outbound msgs & add our ip to be sent to all nodes
    outbound_msg_queue = list_create();

    //Send out our existence
    li_foreach(other_nodes,announce_existance, NULL);

    //Reset node earnings
    node_earnings = 0;

    //Create execution queue
    inbound_msg_queue = list_create();


    //Create workers
    pthread_mutex_t our_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_create(&inbound_network_thread, NULL, in_server, NULL);
    pthread_create(&outbound_network_thread, NULL, out_server, NULL);
    pthread_create(&inbound_executor_thread, NULL, inbound_executor, NULL);

    //Begin mining
    mine();

    return 0;

}
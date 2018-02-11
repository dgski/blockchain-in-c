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

#define DEBUG 0
#define MESSAGE_LENGTH 100000
#define SHORT_MESSAGE_LENGTH 300

/////////////////////////////////////////////////////
//GLOBAL VARIABLES
/////////////////////////////////////////////////////

//Identification
char our_ip[300] = {0};
int our_id = 0;
char node_list_file[300] = {0};
RSA* our_keys;
char* pub_key;
char stripped_pub_key[500];
char* pri_key;

char pri_file[300];
char pub_file[300];
char our_chain_file[300];

//Blockchains
blockchain* our_chain;
dict* foreign_chains;

unsigned int node_earnings;
int expected_length;
int beaten;

//Threads
pthread_t inbound_network_thread;
pthread_t outbound_network_thread;
pthread_t inbound_executor_thread;
pthread_mutex_t our_mutex;
int close_threads;

pthread_t outbound_msgs[10000];
int out_index = 0;

//Lists
list* other_nodes;
list* outbound_msg_queue; //holds outbound message structs
list* inbound_msg_queue; //holds recieved char* messages to execute


//Sockets
int sock_in;
dict* out_sockets;
int last_ping;


/////////////////////////////////////////////////////
//UTILITY FUNCTIONS
/////////////////////////////////////////////////////

//Print remainding balance in quickledger
int print_balance(bt_node* current_node, void* data) {

    if(current_node == NULL || current_node->data == NULL) return 0;

    int* balance = current_node->data;
    printf("%.10s... : %d\n", (current_node->key)+ 12, *balance);
    return 1;
}

//Print remaining keys within quickledger
int print_keys(bt_node* current_node, void* data) {
    
    if(current_node == NULL || current_node->data == NULL) return 0;

    printf("- %s\n", current_node->key);
    return 1;
}

//Print outbound queue
void* print_queue(void* input) {

    if(input == NULL) return NULL;

    message_item* item = (message_item*)input;
    printf("- toWhom: %s, ", item->toWhom);
    printf("Message: %s\n", item->message);
    return NULL;

}

//Empty message struct
void setup_message(message_item* in_message) {
    
    if(in_message == NULL) return;

    in_message->tries = 0;
    memset(in_message->toWhom, 0, sizeof(in_message->toWhom));
    return;
}

//Frees memory of all chains in a dict
int destroy_chains_in_dict(bt_node* current_node, void* data) {

    if(current_node == NULL || current_node->data == NULL) return 0;

    alt_chain* to_destroy = (alt_chain*)current_node->data;
    discard_chain(to_destroy->the_chain);
    
    return 1;
}

//Frees memory of all chains in a dict
int destroy_sockets_in_dict(bt_node* current_node, void* data) {

    if(current_node == NULL || current_node->data == NULL) return 0;
    
    socket_item* the_socket = (socket_item*)current_node->data;

    if(the_socket == NULL) return 0;

    nn_close(the_socket->socket);
    dict_del_elem(out_sockets,current_node->key,0);
    
    return 1;
}

//Print a given nodes value taken from a linked list of strings
void* print_string(void* data) {

    char* string = (char*)data;
    printf("- %s\n", string);

    return NULL;

}


/////////////////////////////////////////////////////
//CORE FUNCTIONS
/////////////////////////////////////////////////////

//Continually search for proper proof of work
int mine() {
    sleep(3);
    printf("\nMining started.\n");
    long result = 0;
    
    while(true) {

        if(DEBUG) {
            char buffer[120] = {0};
            fgets(buffer, sizeof(buffer), stdin);

            if(buffer[0] == 'a')
                graceful_shutdown(1);
        }

        //Generate creation transaction with signature
        if(beaten == 0) {
        pthread_mutex_lock(&our_mutex);
        char sig[513] = {0};
        char output[2500] = {0};
        int time_of_creation = time(NULL);
        if(our_chain->total_currency < CURRENCY_CAP) {
            string_trans_nosig(output, time_of_creation,stripped_pub_key, stripped_pub_key, CURRENCY_SPEED);
            message_signature(sig,output, our_keys,pub_key);
            new_transaction(our_chain, time_of_creation,stripped_pub_key,stripped_pub_key, CURRENCY_SPEED, sig);
        }
        else {
            string_trans_nosig(output, time_of_creation,stripped_pub_key, stripped_pub_key, 0);
            message_signature(sig,output, our_keys,pub_key);
            new_transaction(our_chain,time_of_creation,stripped_pub_key,stripped_pub_key, 0, sig);
        }
        pthread_mutex_unlock(&our_mutex);
        }


        //Actual mining
        unsigned int time_1 = time(NULL);
        result = proof_of_work(&beaten,our_chain->last_hash, our_chain->trans_hash);
        unsigned int time_2 = time(NULL);

        pthread_mutex_lock(&our_mutex);

        //Found necessary proof of work
        if(result > 0 && beaten == 0) {
            printf(ANSI_COLOR_GREEN);
            printf("\nMINED: %.4f min(s)\n", (time_2 - time_1)/60.0);

            our_chain->last_proof_of_work = result;
            blink* a_block = append_current_block(our_chain, our_chain->last_proof_of_work);
            print_block(a_block,'-');

            printf(ANSI_COLOR_RESET);
            li_foreach(other_nodes,announce_length, NULL);

            }

        //Someone beat us
        else if(result == -1 && beaten == 1) {
            printf("Abandoning our in-progress block.\n");
            //memset(our_chain->trans_list,0, sizeof(our_chain->trans_list));
            //our_chain->trans_index = 0;
            beaten = 0;

        }

        //Program is closing
        else if(result == -2) {
            pthread_mutex_unlock(&our_mutex);
            return 0;
        }

        //Access our earnings from the quickledger library

        int our_earnings = 0;
        void* our_ledger_earnings = dict_access(our_chain->quickledger,stripped_pub_key);
        if(our_ledger_earnings != NULL)
            our_earnings += *((int*)our_ledger_earnings);

        printf("\nQUICKLEDGER BALANCES:\n");
        dict_foreach(our_chain->quickledger, print_balance, NULL);

        printf("\nTotal Node Earnings: %d noins\n", our_earnings);
        printf("Total Currency in Circulation: %d noins\n", our_chain->total_currency);
        printf("Verified Transactions: %d\n\n", our_chain->verified->size);

        pthread_mutex_unlock(&our_mutex);

    }
}

//Send out current chain's length to everyone
void* announce_length(list* in_list, li_node* in_item, void* data) {

    if(in_item == NULL) return NULL;
    if(in_item->size > 299) return NULL;

    char data_string[SHORT_MESSAGE_LENGTH];
    memcpy(data_string,in_item->data,in_item->size);
    data_string[in_item->size] = 0;

    if(!strcmp(data_string, our_ip)) {
        return NULL;
    }

    char message[SHORT_MESSAGE_LENGTH];
    sprintf(message,"L %d ",our_chain->length);
    strcat(message, our_ip);

    message_item chain_send;
    setup_message(&chain_send);
    strcpy(chain_send.toWhom, data_string);
    strcpy(chain_send.message, message);

    li_append(outbound_msg_queue,&chain_send, sizeof(chain_send));

    return NULL;
}

//Send out own ip address to everyone in other_nodes list
void* announce_existance(list* in_list, li_node* in_item, void* data) {
    
    if(in_item == NULL || in_item->size > 300) return NULL;

    char data_string[SHORT_MESSAGE_LENGTH];
    memcpy(data_string,in_item->data,in_item->size);
    data_string[in_item->size] = 0;

    if(!strcmp(data_string, our_ip)){
        return NULL;
    }
   
    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom, data_string);
    strcpy(announcement.message, "N ");
    strcat(announcement.message, our_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));

    return NULL;
}

void* announce_message(list* in_list, li_node* in_item, void* data) {

    char* out_message = (char*)data;

    if(in_item == NULL || in_item->size > 300) return NULL;

    char data_string[SHORT_MESSAGE_LENGTH];
    memcpy(data_string,in_item->data,in_item->size);
    data_string[in_item->size] = 0;

    if(!strcmp(data_string, our_ip)){
        return NULL;
    }
   
    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom, data_string);
    strcpy(announcement.message, out_message);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));

    return NULL;


}

void* send_node_list_to(list* in_list, li_node* in_item, void* data) {
    
    char* out_dest = (char*)data;

    if(in_item == NULL || in_item->size > 300) return NULL;

    char data_string[SHORT_MESSAGE_LENGTH];
    memcpy(data_string,in_item->data,in_item->size);
    data_string[in_item->size] = 0;

    if(!strcmp(data_string, our_ip)){
        return NULL;
    }

    char outbound_msg[500] = {0};
    sprintf(outbound_msg,"N %s", data_string);
   
    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom, out_dest);
    strcpy(announcement.message, outbound_msg);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));

    return NULL;
}

//Insert transaction [sender receiver amount]
int insert_trans(const char* input) {

    printf("\nVerifying Transaction...");

    char buffer[TRANS_STRING_LENGTH] = {0};
    strcpy(buffer, input);

    char* time_of = strtok(buffer," ");
    char* sender = strtok(NULL," ");
    char* recipient = strtok(NULL, " ");
    char* amount = strtok(NULL, " ");
    char* signature = strtok(NULL, " ");
    
    //TEMPTEMP disabled
    //Check if user has enough in quick ledger
    void* sender_balance = dict_access(our_chain->quickledger, sender);
    if(sender_balance == NULL || (int)sender_balance < atoi(amount)) {
        printf("Insufficient Funds.\n");
        return 0;
    }

    char message_to_verify[MESSAGE_LENGTH];
    string_trans_nosig(message_to_verify, atoi(time_of),sender, recipient, atoi(amount));

    //Check if transaction is signed properly
    if(!verify_message(message_to_verify, sender, signature))
        return 0;

    printf("\nInserting.\n");

    new_transaction(our_chain, atoi(time_of) ,sender,recipient,atoi(amount),signature);

    return 1;
}

//Verify the format of the post: Should be one 'letter'
int verify_post_format(const char* post) {
    
    if(post == NULL || strlen(post) > 1) return 0;

    if(post[0] > 64 && post[0] < 72)
        return 1;
    
    return 0;
}

//Insert [post] [sender post]
int insert_post(const char* input) {

    printf("\nVerifying Post...");

    char time_of[20] = {0};
    char sender[1000] = {0};
    char music[5] = {0};
    char signature[513] = {0};
    sscanf(input, "%s %s %s %s", time_of, sender, music, signature);

    if(!verify_post_format(music)) {
        printf("Music Format Invalid.");
        return 0;
    }

    //Check if user has enough in quick ledger
    void* sender_balance = dict_access(our_chain->quickledger, sender);

    if(sender_balance == NULL){
        printf("No Funds For Post.\n");
        return 0;
    }

    if((int)sender_balance < 1) {
        printf("Insufficient Funds For Post (%d).\n", (int)sender_balance);
        return 0;
    }

    char data = music[0];
    printf("Note to Add: '%c'\n", data);

    char message[3000];
    sprintf(message, "%s %s %s ", time_of, sender, music);

    if(!verify_message(message, sender,signature)) {
        printf("Post invalid!\n");
        return 0;
    }

    new_post(our_chain, atoi(time_of),sender,music[0],signature);

    return 1;
}

//Create a socket to be used for the given address
int create_socket(const char* input) {

    if(strlen(input) > 299) return 0;

    char address[SHORT_MESSAGE_LENGTH];
    strcpy(address, input);

    printf("Creating socket for... %s\n", address);

    socket_item new_out_socket;

    new_out_socket.last_used = time(NULL);

    new_out_socket.socket = nn_socket(AF_SP, NN_PUSH);
    if(new_out_socket.socket < 0) return 0;
    int timeout = 100;
    if(nn_setsockopt(new_out_socket.socket, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) < 0) return 0;

    if(nn_connect (new_out_socket.socket, address) < 0){
        printf("Connection Error.\n");
        nn_close(new_out_socket.socket);
    }
    
    dict_insert(out_sockets,address,&new_out_socket,sizeof(new_out_socket));

    return 1;
}

//Regster New Node and send out your chain length
int register_new_node(const char* input) {

    if(input == NULL) return 0;

    if(!strcmp(input, our_ip)) {
        printf("Someone is propagating us. Word is spreading.\n");
        return 1;
    }

    if(strlen(input) > SHORT_MESSAGE_LENGTH) {
        printf("IP address too long to register!");
        return 0;
    }

    char add_who[SHORT_MESSAGE_LENGTH] = {0};
    strcpy(add_who,input);
    
    printf("Registering New Node...");
    if(li_search(other_nodes,NULL,add_who,strlen(add_who) + 1)) {
        printf(" Already registered. Updating Last Used Time.\n");

        socket_item* our_socket = (socket_item*)dict_access(out_sockets,add_who);

        if(our_socket != NULL) {
            our_socket->last_used = time(NULL);
        }

    }
    else {
        //Add to our list
        li_append(other_nodes, add_who, strlen(add_who) + 1);

        //Create socket
        create_socket(add_who);

        //Propagate
        printf("Added '%s' ... Propgating.\n", add_who);
        char mess_to_prop[400] = {0};
        sprintf(mess_to_prop,"N %s", add_who);
        li_foreach(other_nodes, announce_message,mess_to_prop); //Send out to all other nodes registered

        //Send our nodes list
        li_foreach(other_nodes, send_node_list_to, add_who);

    }

    printf("Sending chain length to: %s\n ", add_who);
    char message[1000];
    sprintf(message,"L %d ",our_chain->length);
    strcat(message, our_ip);

    message_item chain_send;
    setup_message(&chain_send);
    strcpy(chain_send.toWhom, add_who);
    strcpy(chain_send.message, message);
    
    li_append(outbound_msg_queue,&chain_send, sizeof(chain_send));

    return 1;

}

//Save our chain in .noins format
int save_chain_to_file(blockchain* in_chain, const char* file_name) {

    if(in_chain == NULL || file_name == NULL) return 0;

    printf("Saving our chain to file: '%s'\n", file_name);
    FILE* chain_file = fopen(file_name, "w"); //blockchain_file name
    if(chain_file == NULL) return 0;

    blink* temp = in_chain->head;

    while(temp != NULL) {
        printf("%d ",temp->data.index);
        char block_to_write[BLOCK_STR_SIZE];
        string_block(block_to_write,&temp->data);
        strcat(block_to_write,"\n");
        fwrite(block_to_write,1,strlen(block_to_write), chain_file);

        temp = temp->next;
    }
    printf("\n");
    fclose(chain_file);
    printf("Done.\n");

    return 1;

}

//Verify the given block read from a a '.noins' file
int verify_file_block(const char* input, int* curr_index) {


    char f_block[MESSAGE_LENGTH] = {0};
    strcpy(f_block, input);
    
    char* index = strtok(f_block, ".");
    char* time_gen = strtok(NULL, ".");
    char* posts = strtok(NULL,".");
    char* posts_size = strtok(NULL, ".");
    char* transactions = strtok(NULL, ".");
    char* trans_size = strtok(NULL, ".");
    char* proof = strtok(NULL, ".");
    char* hash = strtok(NULL, ".");

    if( atoi(index) != 0 && (index == NULL || time_gen == NULL || posts == NULL || 
    posts_size == NULL || transactions == NULL || trans_size == NULL ||
    proof == NULL || hash == NULL))
        return 0;

    printf("Block with index %d recieved\n", atoi(index));


    //First time encountering this chain - came with index 0
    if(atoi(index) == 0) {
        printf("Creating chain for this block.\n");
        our_chain = new_chain();
        *(curr_index) = *(curr_index) + 1 ;
        return 1;
    }
    //First time encountering this chain - came with index > 0
    else if(our_chain == NULL && atoi(index) != 0) {
        printf("No chain for non-zero index block.\n");
        return 0;
    }
    //Block came with index not expected by sequencer
    if(atoi(index) != *curr_index) {
        printf("Not expecting this index number\n");
        return 0;
    }
    // Everything good... Get ready for next block
    else {
        *(curr_index) = *(curr_index) + 1 ;
    }

    long the_proof;
    sscanf(proof,"%020ld",&the_proof);
    printf("Recieved Proof: %ld\n", the_proof);
    printf("Index Recieved: %d\n",atoi(index));
    printf("Current length: %d\n",our_chain->length);
    printf("Last Hash: %s\n",our_chain->last_hash);

    char trans_hash[HASH_HEX_SIZE];
    transaction new_trans_array[20] = {0};

    extract_transactions_raw(new_trans_array,transactions);

    post new_post_array[BLOCK_DATA_SIZE] = {0};
    int nr_of_posts = extract_posts_raw(new_post_array,posts);

    hash_transactions(trans_hash,new_trans_array, atoi(trans_size), new_post_array, atoi(posts_size));

    //Chain is valid
    if(valid_proof(our_chain->last_hash,trans_hash, the_proof)){

        transaction rec_trans[20] = {0};
        int all_valid_trans = extract_transactions(our_chain, rec_trans, transactions);
        int all_valid_posts = validate_posts(our_chain,new_post_array,atoi(posts_size));

        if(!all_valid_trans && !all_valid_posts) {
            printf("Invalid.\n");
            return 0;
        }
        printf("Valid.\n");


        //Add block
        blink* a_block = append_new_block(our_chain, atoi(index), atoi(time_gen),rec_trans, new_post_array, atoi(trans_size), atoi(posts_size), the_proof);

        printf(ANSI_COLOR_YELLOW);
        printf("READ BLOCK:\n");
        print_block(a_block,'-');
        printf(ANSI_COLOR_RESET);
        return 1;

    }
    else {
        printf("Invalid.\n");
        return 0;
    }

    return 0;


}

//Open the given file, read it line by line, and validate the blocks
int read_chain_from_file(blockchain* in_chain, const char* file_name) {

    printf("Reading our chain from file: '%s'\n", file_name);
    FILE* chain_file = fopen(file_name, "r"); //blockchain_file name
    if(chain_file == NULL) return 0;

    char buffer[BLOCK_STR_SIZE] = {0};
    int curr_index = 0;
    while (fgets(buffer, sizeof(buffer), chain_file)) {
        if(buffer[strlen(buffer) -1] == '\n') buffer[strlen(buffer) -1] = 0;
        printf("Read from file: %s\n", buffer);
        int suc_read = verify_file_block(buffer,&curr_index);

        if(!suc_read) {
            discard_chain(our_chain);
            return 0;
        }
    }
    fclose(chain_file);

    return 1;


}

//Send request for complete chain to address of claimant
int request_chain(const char* address) {

    if(address == NULL) return 0;

    printf("Requesting chain from: %s, ", address);
    char message[SHORT_MESSAGE_LENGTH];
    strcpy(message, "C ");
    strcat(message, our_ip);

    message_item request_chain;
    setup_message(&request_chain);
    strcpy(request_chain.toWhom, address);
    strcpy(request_chain.message, message);
    
    li_append(outbound_msg_queue,&request_chain, sizeof(request_chain));

    return 1;

}

//Compare received chain length to our length, if bigger request more
int compare_length(const char* input) {

    if(input == NULL) return 0;

    if(strlen(input) > 200) return 0;

    printf("Checking foreign chain length... ");
    unsigned int foreign_length;
    char Whom[300] = {0};
    sscanf(input, "%d %s", &foreign_length, Whom);

    if(foreign_length > our_chain->length) {
        printf("Someone claims they are a winner!\n");
        request_chain(Whom);
    }
    else
        printf("False alarm. Our chain is still good!\n");

    return 0;
}

//Send out our complete chain as block messages
int send_our_chain(const char* address) {

    if(address == NULL) return 0;
    
    //Setup
    if(strlen(address) > 200) return 0;
    printf("Sending blockchain to: %s, ", address);
    char message[MESSAGE_LENGTH] = {0};
    if(our_chain->head == NULL) return 0;
    blink* temp = our_chain->head;
    
    //Generate random chain id
    char rand_chain_id[60];
    srand(time(NULL));
    int r = rand();   
    sprintf(rand_chain_id, "CHIN%010d%010d.", r, our_id);
    printf("Generated chain: '%s'\n", rand_chain_id);

    //Iterate over all blocks in chain
    for(int i = 0; i < our_chain->length + 1; i++) {
        
        strcpy(message, "B "); //Add message type

        strcat(message, rand_chain_id); //Add random chain id
        char the_length[12];
        sprintf(the_length,"%010d.", our_chain->length);
        strcat(message, the_length); //Add chain length
        char block[BLOCK_STR_SIZE] = {0};
        string_block(block,&temp->data);
        strcat(message, block); //Add block

        message_item the_chain;
        setup_message(&the_chain);
        strcpy(the_chain.toWhom, address);
        strcpy(the_chain.message, message);
        
        li_append(outbound_msg_queue,&the_chain, sizeof(the_chain));

        temp = temp->next;
    }

    return 1;
}

//Remove chains that have no been used for a minute: They're old now
int prune_chains(bt_node* current_node, void* data) {

    if(current_node == NULL || current_node->data == NULL /*|| data == NULL */) return 0;

    alt_chain* the_chain = (alt_chain*)current_node->data;
    list* pruner_list = (list*)data;

    if( time(NULL) - the_chain->last_time > 60 ) {
        printf("Pruning chain with ID: '%s'\n", current_node->key);
        discard_chain(the_chain->the_chain);
        //dict_del_elem(foreign_chains,current_node->key,0);
        li_append(pruner_list, current_node->key, strlen(current_node->key) + 1);
    }

    return 1;
}

//Remove Sockets that have not been used for 600 seconds: They're old now
int prune_sockets(bt_node* current_node, void* data) {

    if(current_node == NULL || current_node->data == NULL) return 0;
    
    socket_item* the_socket = (socket_item*)current_node->data;
    list* pruner_list = (list*)data;

    if(the_socket == NULL) return 0;

    if( time(NULL) - the_socket->last_used > 600 ) {
        printf("Pruning socket with ip: '%s'\n", current_node->key);
        nn_close(the_socket->socket);
        //dict_del_elem(out_sockets,current_node->key,0);
        li_append(pruner_list, current_node->key,  strlen(current_node->key) + 1);

    }

    return 1;
}

//Check whether the block received is valid
int verify_foreign_block(const char* input) {

    
    char f_block[MESSAGE_LENGTH] = {0};
    strcpy(f_block, input);
    
    char* chain_id = strtok(f_block,".");
    char* exp_length = strtok(NULL, ".");
    char* index = strtok(NULL, ".");
    char* time_gen = strtok(NULL, ".");
    char* posts = strtok(NULL,".");
    char* posts_size = strtok(NULL, ".");
    char* transactions = strtok(NULL, ".");
    char* trans_size = strtok(NULL, ".");
    char* proof = strtok(NULL, ".");
    char* hash = strtok(NULL, ".");

    printf("Block of %s with index %d recieved\n", chain_id, atoi(index));

    alt_chain* this_chain = dict_access(foreign_chains,chain_id);

    //First time encountering this chain - came with index 0
    if(this_chain == NULL && atoi(index) == 0) {

        printf("Creating chain for this block.\n");
        alt_chain new_for_chain;
        new_for_chain.expected_length = atoi(exp_length);
        new_for_chain.expected_index = 1;
        new_for_chain.last_time = time(NULL);
        new_for_chain.the_chain = new_chain();
        dict_insert(foreign_chains,chain_id,&new_for_chain,sizeof(new_for_chain));

        return 1;
    }
    //First time encountering this chain - came with index > 0
    else if(this_chain == NULL && atoi(index) != 0) {
        printf("No chain for non-zero index block.\n");
        return 0;
    }

    if(this_chain == NULL) return 0;

    //Block came with index not expected by sequencer
    if(atoi(index) != this_chain->expected_index) {
        printf("Not expecting this index number\n");
        return 0;
    }


    long the_proof;
    sscanf(proof,"%020ld",&the_proof);
    printf("Recieved Proof: %ld\n", the_proof);
    printf("Index Recieved: %d\n",atoi(index));
    printf("Expected length: %d\n",this_chain->expected_length);
    printf("Our length: %d\n",our_chain->length);
    printf("Last Hash Length: %lu\n", strlen(this_chain->the_chain->last_hash));
    printf("Last Hash: %s\n",this_chain->the_chain->last_hash);

    char trans_hash[HASH_HEX_SIZE];
    transaction new_trans_array[20] = {0};

    int ret = extract_transactions_raw(new_trans_array,transactions);
    if(!ret) return 0;

    post new_post_array[BLOCK_DATA_SIZE] = {0};
    int nr_of_posts = extract_posts_raw(new_post_array,posts);

    hash_transactions(trans_hash,new_trans_array, atoi(trans_size), new_post_array, atoi(posts_size));



    //Block is valid
    if(valid_proof(this_chain->the_chain->last_hash,trans_hash, the_proof)){

        this_chain->last_time = time(NULL);

        transaction rec_trans[20] = {0};
        int all_valid_trans = extract_transactions(this_chain->the_chain, rec_trans, transactions);
        int all_valid_posts = validate_posts(this_chain->the_chain,new_post_array,atoi(posts_size));


        if(!all_valid_trans || !all_valid_posts) {
            printf("Invalid.\n");
            return 0;
        }
        printf("Valid.\n");

        
        //Add block
        blink* a_block = append_new_block(this_chain->the_chain, atoi(index), atoi(time_gen),rec_trans, new_post_array, atoi(trans_size), atoi(posts_size), the_proof);

        printf("\nFOREIGN DICT SIZE %d:\n", foreign_chains->size);
        dict_foreach(foreign_chains, print_keys,NULL);
        printf("\n");

        this_chain->expected_index++;



        if((atoi(index) == this_chain->expected_length) ) {
            
            if(atoi(index) > our_chain->length) {
                printf("\nComplete chain is verified and longer. Make that ours.\n");

                printf(ANSI_COLOR_CYAN);
                printf("\nABSORBED TOP BLOCK:\n");
                print_block(a_block,'-');
                printf(ANSI_COLOR_RESET);

                discard_chain(our_chain);
                our_chain = this_chain->the_chain;
                dict_del_elem(foreign_chains,chain_id,0);
            
                beaten = 1;
            }
            else {
                printf("Received end of chain. Not longer. Throwing away.\n");
                discard_chain(this_chain->the_chain);
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

    if(in_msg == NULL) return;

    char to_process[MESSAGE_LENGTH] = {0};
    strcpy(to_process, in_msg);

    char* token = strtok(to_process," ");

    if(!strcmp(token, "T"))
        insert_trans(to_process + 2);
    if(!strcmp(token, "P"))
        insert_post(to_process + 2);
    if(!strcmp(token, "B"))
        verify_foreign_block(to_process + 2);
    if(!strcmp(token, "N"))
        register_new_node(to_process + 2);
    if(!strcmp(token, "L"))
        compare_length(to_process + 2);
    if(!strcmp(token, "C"))
        send_our_chain(to_process + 2);
    if(!strcmp(token, "V"))
        verify_acceptance_trans_or_post(to_process + 2);
    if(!strcmp(token, "I"))
        client_existance_announcement(to_process + 2);

}

//Check whether the given signaure exists in your verified dictionary
int verify_acceptance_trans_or_post(const char* input) {

    printf("Checking acceptance of transaction...\n");

    char ip_address_out[300];
    char signature[513];

    sscanf(input,"%s %s", ip_address_out, signature);

    void* entry= dict_access(our_chain->verified,signature);

    if(entry == NULL)
        return 0;

    //Reply that transaction is included in our chain
    message_item confirmation;
    setup_message(&confirmation);
    strcpy(confirmation.toWhom, ip_address_out);
    strcpy(confirmation.message, "V ");
    strcat(confirmation.message, signature);
    strcat(confirmation.message, " ");
    strcat(confirmation.message, our_ip);

    li_append(outbound_msg_queue,&confirmation,sizeof(confirmation));

    return 1;

}

//Send out a ping to all other nodes every 30 seconds 
int ping_function() {

    if(time(NULL) - last_ping > 30) {
        printf("Pinging Nodes in List...\n");
        li_foreach(other_nodes,announce_existance, NULL);
        last_ping = time(NULL);
    }

    return 1;
}

//Send node list to this client
int client_existance_announcement(const char* input) {

    if(input == NULL) return 0;

    char ip_address_out[300];
    strcpy(ip_address_out,input);
    
    li_foreach(other_nodes, send_node_list_to, ip_address_out);
    return 1;
}

//Inbound thread function - receives messages and adds them to execution queue
void* in_server() {
    
    printf("noincoin: Server v1.0\n");
    printf("Node IP: %s\n", our_ip);
    printf("Other node List: %s\n", node_list_file);
    printf("Pri Key File: %s\n", pri_file);
    printf("Pub Key File: %s\n", pub_file);
    printf("Chain File: %s\n", our_chain_file);


    sock_in = nn_socket (AF_SP, NN_PULL);
    assert (sock_in >= 0);
    assert (nn_bind (sock_in, our_ip) >= 0);
    int timeout = 50;
    assert (nn_setsockopt (sock_in, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof (timeout)) >= 0);

    printf("Inbound Socket Ready!\n");

    char buf[MESSAGE_LENGTH];

    while(true) {
        int bytes = nn_recv(sock_in, buf, sizeof(buf), 0);
        
        pthread_mutex_lock(&our_mutex);
        if(bytes > 0) {
            buf[bytes] = 0;
            printf("\nRecieved %d bytes: \"%s\"\n", bytes, buf);

            li_append(inbound_msg_queue,buf,bytes);
        }
        if(close_threads) {
                pthread_mutex_unlock(&our_mutex);            
                return 0;
        }


        pthread_mutex_unlock(&our_mutex);            

    }
    return 0;
}

//Free the given data within a list of dicts
void* destroy_items_in_dict(list* in_list, li_node* input , void* data) {

    if(in_list == NULL || input == NULL || data == NULL) return NULL;

    char* to_destroy = (char*)input->data;
    dict* for_chain_dict = (dict*)data;

    dict_del_elem(for_chain_dict,to_destroy, 0);

    return NULL;
}

//Executes everything in execution queue + prunes data structures
void* inbound_executor() {
    while(true) {
        li_foreach(inbound_msg_queue, process_inbound, &our_mutex);


        pthread_mutex_lock(&our_mutex);
        list* chains_to_prune = list_create();
        dict_foreach(foreign_chains,prune_chains,chains_to_prune);
        li_foreach(chains_to_prune, destroy_items_in_dict, foreign_chains);
        if(chains_to_prune->length > 0) {
            printf("CHAINS IN PRUNE LIST:\n");
            li_print(chains_to_prune, print_string);
        }
        li_discard(chains_to_prune);

        list* sockets_to_prune = list_create();
        dict_foreach(out_sockets,prune_sockets,sockets_to_prune);
        li_foreach(sockets_to_prune, destroy_items_in_dict, out_sockets);
        if(sockets_to_prune->length > 0) {
            printf("SOCKETS IN PRUNE LIST:\n");
            li_print(sockets_to_prune, print_string);
        }
        li_discard(sockets_to_prune);


        if(close_threads) {
            pthread_mutex_unlock(&our_mutex);
            return NULL;
        }



        pthread_mutex_unlock(&our_mutex);
        
        usleep(100);
    }
}

//Executed the message, input is of type message_item struct
void* process_inbound(list* in_list, li_node* input, void* data) {

    if(input == NULL || data == NULL) return NULL;

    pthread_mutex_t* the_mutex = (pthread_mutex_t*)data;


    char the_message[MESSAGE_LENGTH] = {0};
    memcpy(the_message,input->data,input->size);


    pthread_mutex_lock(the_mutex);
    process_message(the_message);
    li_delete_node(in_list, input);
    pthread_mutex_unlock(the_mutex);

    return NULL;
}

//Outbound thread function - tries to send everything in outbound message queue
void* out_server() {
    while(true) {
        pthread_mutex_lock(&our_mutex);

        pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;
        
        li_foreach(outbound_msg_queue, process_outbound, &message_mutex);

        ping_function();
        if(close_threads) {
            pthread_mutex_unlock(&our_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&our_mutex);

        usleep(100);
    }
}

//Spawns a random socket for use for one time - not a usual customer
int rare_socket(message_item* in_message) {

    int sock_here = nn_socket(AF_SP, NN_PUSH);
    assert (sock_here >= 0);
    int timeout = 100;
    assert (nn_setsockopt(sock_here, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);

    //printf("Sending to: %s: '%.10s'",our_message->toWhom, our_message->message);
    if(nn_connect (sock_here, in_message->toWhom) < 0){
        printf("Connection Error.\n");
        nn_close(sock_here);
    }

    return sock_here;
}

//Done to each message in 'outbound_msg_queue'. input is of type message_item struct
void* process_outbound(list* in_list, li_node* input, void* data) {
    
    if(input == NULL) return NULL;

    message_item* our_message = (message_item*)input->data;
    socket_item* sock_out_to_use = (socket_item*)dict_access(out_sockets,our_message->toWhom);

    if(our_message->tries == 1) return NULL;

    int the_socket;
    int used_rare_socket = 0;

    if(sock_out_to_use == NULL) {
        the_socket = rare_socket(our_message);
        used_rare_socket = 1;
    }
    else {
        the_socket = sock_out_to_use->socket;
    }


    printf("Sending to: %s, ",our_message->toWhom);
    int bytes = nn_send (the_socket, our_message->message, strlen(our_message->message), 0);
    printf("Bytes sent: %d\n", bytes);

    if(used_rare_socket) {
        usleep(100000);
        nn_close(the_socket);
    }
    else {
        usleep(100);
    }

    //Try three times
    if(bytes > 0 || our_message->tries == 2) li_delete_node(in_list, input);
    else our_message->tries++;

    return NULL;
}

//Read configuration file
int read_node_list() {

    printf("Using configuration file: '%s'\n", node_list_file);

    FILE* config = fopen(node_list_file, "r");
    if(config == NULL) return 0;

    printf("Reading configuration file...\n");

    char buffer[300] = {0};
    while (fgets(buffer, sizeof(buffer), config)) {
        if(buffer[strlen(buffer) -1] == '\n') buffer[strlen(buffer) -1] = 0;

        //Add to list
        li_append(other_nodes, buffer, strlen(buffer) + 1);

        //Create socket
        create_socket(buffer);
    }
    fclose(config);

    return 1;
}

//For given node data, write to the file provided
void* li_write_string_file(list* in_list, li_node* in_node, void* data) {

    if(in_node == NULL || in_node->size > 300) return NULL;
    FILE* our_config = (FILE*)data;

    char string[300] = {0};
    memcpy(string,in_node->data,in_node->size);
    string[in_node->size] = '\n';
    printf("Saving to file: '%s'\n", string);
    fwrite(string , 1 , strlen(string) + 2 , our_config);

    return NULL;
}

//Write configuration file
int write_config() {
    printf("Saving ip addresses to file '%s': \n", node_list_file);
    FILE* config = fopen(node_list_file, "w"); //node_list_file
    if(config == NULL) return 0;

    li_foreach(other_nodes,li_write_string_file,config);

    fclose(config);

    return 0;

}

//Free all outstanding memory
void graceful_shutdown(int dummy) {

    printf("\nCommencing graceful shutdown!\n");

    close_threads = 1;
    beaten = 2;


    pthread_join(outbound_network_thread, NULL);
    pthread_join(inbound_network_thread, NULL);
    pthread_join(inbound_executor_thread, NULL);

    pthread_mutex_lock(&our_mutex);


    //Write out new node list to file
    write_config();
    
    //Discard lists
    li_discard(other_nodes);
    li_discard(outbound_msg_queue);
    li_discard(inbound_msg_queue);

    //Save blockchain to file
    save_chain_to_file(our_chain, our_chain_file);

    //Discard blockchains
    discard_chain(our_chain);
    dict_foreach(foreign_chains, destroy_chains_in_dict, NULL);
    dict_foreach(foreign_chains, destroy_sockets_in_dict, NULL);
    dict_discard(foreign_chains);
    

    //Discard keys
    destroy_keys(&our_keys, &pri_key, &pub_key);

    nn_close(sock_in);

    pthread_mutex_unlock(&our_mutex);

    pthread_mutex_destroy(&our_mutex);

    exit(0);
}

//Save our ip address 
int setup_ip_address(const char* in_ip_address) {

    if(in_ip_address == NULL) return 0;

    else if(strlen(in_ip_address) < 300){
        strcpy(our_ip, in_ip_address);
        return 1;
    }

    printf("Provide a valid address with a length less than 300 characters.\n");
    return 0;
}

//Read private key filename from command line argument
int setup_pri_key(const char* in_pri_key) {

    if(in_pri_key == NULL) return 0;

    //Get our config file from argument
    if(strlen(in_pri_key) < 300) {
        strcpy(pri_file, in_pri_key);
        return 1;
    }

    printf("Provide a valid pub key file with a named length of less than 300 characters.\n");
    return 0;
}

//Read public key filename from command line argument
int setup_pub_key(const char* in_pub_key) {

    if(in_pub_key == NULL) return 0;

    //Get our config file from argument
    if(strlen(in_pub_key) < 300) {
        strcpy(pub_file, in_pub_key);
        return 1;
    }

    printf("Provide a valid pub key file with a named length of less than 300 characters.\n");
    return 0;
}

//Read node list filename from command line argument
int setup_node_list(const char* in_node_list) {

    if(in_node_list == NULL) return 0;

    //Get our config file from argument
    if(strlen(in_node_list) < 300) {
        strcpy(node_list_file, in_node_list);
        return 1;
    }

    printf("Provide a valid node file path with a length less than 300 characters.\n");
    return 0;

}

//Read chain name filename from command line argument
int setup_chain_file(const char* in_chain_file) {

    if(in_chain_file == NULL) return 0;

    //Get our config file from argument
    if(strlen(in_chain_file) < 300) {
        strcpy(our_chain_file, in_chain_file);
        return 1;
    }

    printf("Provide a valid node file path with a length less than 300 characters.\n");
    return 0;

}

//Parses and processes commandline arguments
int command_line_parser(int argc, const char* argv[]) {

    if(argv == NULL) return 0;

    //Process commandline arguments (skip #1)
    for(int i = 1; i < argc; i = i + 2) {

        if(argv[i] == NULL || argv[i + 1] == NULL)
            return 0;

        if(!strcmp(argv[i],"-i")){
            if(!setup_ip_address(argv[i + 1]))
                return 0;
        }
        else if(!strcmp(argv[i],"-n")){
            if(!setup_node_list(argv[i + 1]))
                return 0;
        }
        else if(!strcmp(argv[i],"-pri")) {
            if(!setup_pri_key(argv[i + 1]))
                return 0;
        }
        else if(!strcmp(argv[i],"-pub")) {
            if(!setup_pub_key(argv[i + 1]))
                return 0;
        }
        else if(!strcmp(argv[i], "-c")) {
            if(!setup_chain_file(argv[i + 1]))
                return 0;
        }
        else {
            return 0;
        }

    }

    return 1;
}

/////////////////////////////////////////////////////
//MAIN FUNCTION
/////////////////////////////////////////////////////

//The star of the show
int main(int argc, const char* argv[]) {
    
    //Ctrl-C Handler
    signal(SIGINT, graceful_shutdown);

    
    beaten = 0;

    //Create random_id
    srand(time(NULL));
    our_id = rand();

    //Initialize Crypto
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    ERR_load_crypto_strings();

    //Load defaults
    strcpy(our_ip, "ipc:///tmp/pipeline_0.ipc");
    strcpy(node_list_file, "node.cfg");
    strcpy(pri_file, "pri_0.pem");
    strcpy(pub_file, "pub_0.pem");
    strcpy(our_chain_file, "chain_0.noins");

    //Create foreign chain dict
    foreign_chains = dict_create();

    //Create list of other nodes
    other_nodes = list_create();

    //Create sockets dictionary
    out_sockets = dict_create();

    //Process commandline arguments
    int setup = command_line_parser(argc, argv);
    if(!setup) {
        printf("Usage: ./node -i {ip_address} -n {node_list} -pri {private_key} -pub {public_key}\n");
        return 0;
    }

    ////Create our blockchain and Process chain file
    int chain_good = read_chain_from_file(our_chain, our_chain_file);
    if(!chain_good) {
        our_chain = new_chain();
    }
    
    //Read in node_list
    int success = read_node_list();
    if(success) {
        printf("All good.\n");
    }
    else {
        printf("Please provide a proper node list file.\n");
        return 0;
    }
    
    //Try to read keys first
    int keys_good = read_keys(&our_keys,pri_file, pub_file);
    if(keys_good == 0){
        RSA_free(our_keys);
        our_keys = NULL;

    }
    create_keys(&our_keys,&pri_key,&pub_key);
    //Otherwise Generate our pri/pub address keys
    if(keys_good) {
        printf("Read Keypair:\n\n");
    }
    else {
        write_keys(&our_keys,pri_file,pub_file);
        printf("Created Keypair (Now Saved):\n\n");
    } 
    strip_pub_key(stripped_pub_key, pub_key);
    printf("%s%s\n\n", pri_key, pub_key);


    //Create list of outbound msgs & add our ip to be sent to all nodes
    outbound_msg_queue = list_create();

    //Send out our existence
    li_foreach(other_nodes,announce_existance, NULL);
    last_ping = time(NULL);

    //Reset node earnings
    node_earnings = 0;

    //Create execution queue
    inbound_msg_queue = list_create();

    //Create workers
    //pthread_mutex_t our_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&our_mutex, NULL);
    pthread_create(&inbound_network_thread, NULL, in_server, NULL);
    pthread_create(&outbound_network_thread, NULL, out_server, NULL);
    pthread_create(&inbound_executor_thread, NULL, inbound_executor, NULL);
    close_threads = 0;

    //Begin mining
    mine();

    return 0;

}
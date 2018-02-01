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
#define MESSAGE_LENGTH 100000
#define SHORT_MESSAGE_LENGTH 300

/////////////////////////////////////////////////////
//GLOBAL VARIABLES
/////////////////////////////////////////////////////

//Identification
char our_ip[300] = {0};
char config_file[300] = {0};
RSA* our_keys;
char* pub_key;
char stripped_pub_key[500];
char* pri_key;

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

//Lists
list* other_nodes;
list* outbound_msg_queue; //holds outbound message structs
list* inbound_msg_queue; //holds recieved char* messages to execute


//Sockets
int sock_in;
int sock_out;



/////////////////////////////////////////////////////
//UTILITY FUNCTIONS
/////////////////////////////////////////////////////

//Print remainding balance in quickledger
int print_balance(bt_node* current_node) {

    if(current_node == NULL || current_node->data == NULL) return 0;

    int* balance = current_node->data;
    printf("%.10s... : %d\n", (current_node->key)+ 12, *balance);
    return 1;
}

//Print remaining keys within quickledger
int print_keys(bt_node* current_node) {
    
    if(current_node == NULL || current_node->data == NULL) return 0;

    printf("%s,", current_node->key);
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

//Empty message
void setup_message(message_item* in_message) {
    
    in_message->tries = 0;
    memset(in_message->toWhom, 0, sizeof(in_message->toWhom));
    return;
}

//Frees memory of all chains in a dict
int destroy_chains_in_dict(bt_node* current_node) {

    if(current_node == NULL) return 0;

    alt_chain* to_destroy = (alt_chain*)current_node->data;
    discard_chain(to_destroy->the_chain);
    
    return 1;
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
        }

        //Generate creation transaction with signature
        if(beaten == 0) {
        pthread_mutex_lock(&our_mutex);
        char sig[513] = {0};
        char output[2500] = {0};
        if(our_chain->total_currency < CURRENCY_CAP) {
            string_trans_nosig(output, stripped_pub_key, stripped_pub_key, CURRENCY_SPEED);
            message_signature(sig,output, our_keys,pub_key);
            new_transaction(our_chain,stripped_pub_key,stripped_pub_key, CURRENCY_SPEED, sig);
        }
        else {
            string_trans_nosig(output, stripped_pub_key, stripped_pub_key, 0);
            message_signature(sig,output, our_keys,pub_key);
            new_transaction(our_chain,stripped_pub_key,stripped_pub_key, 0, sig);
        }
        pthread_mutex_unlock(&our_mutex);
        }


        //Actual mining
        unsigned int time_1 = time(NULL);
        result = proof_of_work(&beaten,our_chain->last_hash, our_chain->trans_hash);
        unsigned int time_2 = time(NULL);

        //Found necessary proof of work
        if(result > 0) {
            printf(ANSI_COLOR_GREEN);
            printf("\nMINED: %.4f min(s)\n", (time_2 - time_1)/60.0);

            our_chain->last_proof_of_work = result;

            blink* a_block = append_current_block(our_chain, our_chain->last_proof_of_work);
            print_block(a_block,'-');

            printf(ANSI_COLOR_RESET);
            li_foreach(other_nodes,announce_length, NULL);

            }

        //Someone beat us
        else if(result == -1) {
            printf("Abandoning our in-progress block.\n");
            memset(our_chain->trans_list,0, sizeof(our_chain->trans_list));
            our_chain->trans_index = 0;
            beaten = 0;
        }

        //Program is closing
        else if(result == -2) {
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
        printf("Total Currency in Circulation: %d noins\n\n", our_chain->total_currency);


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

    pthread_mutex_lock(&our_mutex);
    li_append(outbound_msg_queue,&chain_send, sizeof(chain_send));
    pthread_mutex_unlock(&our_mutex);

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

    pthread_mutex_lock(&our_mutex);
    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
    pthread_mutex_unlock(&our_mutex);

    return NULL;
}

//Insert transaction [sender receiver amount]
int insert_trans(const char* input) {

    printf("\nVerifying Transaction...");

    char buffer[TRANS_STRING_LENGTH] = {0};
    strcpy(buffer, input);

    char* sender = strtok(buffer," ");
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
    if(!verify_signiture(buffer,sender, recipient, amount, signature))
        return 0;

    printf("\nInserting.\n");

    int amount_int;
    sscanf(amount,"%d",&amount_int);

    new_transaction(our_chain,sender,recipient,amount_int,signature);

    printf("AFTER ADDING: %s\n", our_chain->trans_list[0].signature);

    return 1;
}

//Insert [post] [sender post]
int insert_post(const char* input) {

    char sender[1000] = {0};
    char music[5] = {0};
    char signature[513] = {0};
    sscanf(input, "%s %s %s", sender, music, signature);

    printf("%s, LENGTH OF: %lu\n", music, strlen(music));

    if(strlen(music) > 1) {
        printf("Music Format Invalid.");
        return 0;
    }

    //Check if user has enough in quick ledger
    void* sender_balance = dict_access(our_chain->quickledger, sender);
    if(sender_balance == NULL || (int)sender_balance < 1) {
        printf("Insufficient Funds For Post.\n");
        return 0;
    }

    char data = music[0];
    printf("NOTE TO ADD: '%c'\n", data);

    char message[2000];
    sprintf(message, "%s %s", sender, music);

    if(!verify_message(message, sender,signature)) {
        printf("Post invalid!\n");
        return 0;
    }

    new_post(our_chain,sender,music[0],signature);

    return 1;
}

//Regster New Node and send out your chain length
void register_new_node(const char* input) {

    if(strlen(input) > SHORT_MESSAGE_LENGTH) {
        printf("IP address too long to register!");
        return;
    }

    char add_who[SHORT_MESSAGE_LENGTH];
    strcpy(add_who,input);
    
    printf("Registering New Node...");
    if(li_search(other_nodes,NULL,add_who,strlen(add_who) + 1)) {
        printf(" Already registered. ");
    }
    else {
        li_append(other_nodes, add_who, strlen(add_who) + 1);
        printf("Added '%s'\n", add_who);
    }

    printf("Sending chain length to: %s\n ", add_who);
    char message[1000];
    sprintf(message,"L %d ",our_chain->length);
    strcat(message, our_ip);

    message_item chain_send;
    setup_message(&chain_send);
    strcpy(chain_send.toWhom, input);
    strcpy(chain_send.message, message);
    
    li_append(outbound_msg_queue,&chain_send, sizeof(chain_send));

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

    return 0;

}

//Compare received chain length to our length, if bigger request more
int compare_length(const char* input) {

    if(input == NULL) return 0;

    if(strlen(input) > 200) return 0;

    printf("Checking foreign chain length... ");
    unsigned int foreign_length;
    char Whom[300];
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
    int r = rand();   
    sprintf(rand_chain_id, "CHIN%010d.", r);

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

int prune_chains(bt_node* current_node) {

    if(current_node == NULL || current_node->data == NULL) return 0;

    if( (time(NULL) - ((alt_chain*)current_node->data)->last_time) > 60 ) {
        printf("Pruning chain with ID: '%s'\n", current_node->key);
        dict_del_elem(foreign_chains,current_node->key,0);
    }

    return 1;
}


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
    //Block came with index not expected by sequencer
    if(atoi(index) != this_chain->expected_index) {
        printf("Not expecting this index number\n");
        return 0;
    }
    // Everything good... Get ready for next block
    else {
        this_chain->expected_index++;
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

    extract_transactions_raw(new_trans_array,transactions);

    post new_post_array[BLOCK_DATA_SIZE] = {0};
    int nr_of_posts = extract_posts_raw(new_post_array,posts);

    hash_transactions(trans_hash,new_trans_array, atoi(trans_size), new_post_array, atoi(posts_size));



    //Chain is valid
    if(valid_proof(this_chain->the_chain->last_hash,trans_hash, the_proof)){

        this_chain->last_time = time(NULL);

        transaction rec_trans[20] = {0};
        int all_valid_trans = extract_transactions(this_chain->the_chain, rec_trans, transactions);
        int all_valid_posts = validate_posts(this_chain->the_chain,new_post_array,atoi(posts_size));

        if(!all_valid_trans && !all_valid_posts) {
            printf("Invalid.\n");
            return 0;
        }
        printf("Valid.\n");


        
        //Add block
        blink* a_block = append_new_block(this_chain->the_chain, atoi(index), atoi(time_gen),rec_trans, new_post_array, atoi(trans_size), atoi(posts_size), the_proof);

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
            
                beaten = 1;
            }
            else {
                printf("Received end of chain. Not longer. Throwing away.\n");
                dict_del_elem(foreign_chains,chain_id,1);
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

}

//Inbound thread function - receives messages and adds them to execution queue
void* in_server() {
    
    printf("Blockchain in C Major: Server v0.1\n");
    printf("Node IP: %s\n\n", our_ip);

    sock_in = nn_socket (AF_SP, NN_PULL);
    assert (sock_in >= 0);
    assert (nn_bind (sock_in, our_ip) >= 0);
    int timeout = 500;
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

        if(close_threads)
                return 0;
        pthread_mutex_unlock(&our_mutex);            

    }
    return 0;
}

//Executes everything in execution queue + prunes data structures
void* inbound_executor() {
    while(true) {
        pthread_mutex_lock(&our_mutex);
        li_foreach(inbound_msg_queue, process_inbound, NULL);
        dict_foreach(foreign_chains,prune_chains,NULL);
        if(close_threads)
            return NULL;
        pthread_mutex_unlock(&our_mutex);
        
        sleep(1);
    }
}

//Executed the message, input is of type message_item struct
void* process_inbound(list* in_list, li_node* input, void* data) {

    if(input == NULL) return NULL;

    char the_message[MESSAGE_LENGTH] = {0};
    memcpy(the_message,input->data,input->size);
    process_message(the_message);

    li_delete_node(in_list, input);

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


//Read configuration file
int read_config() {

    printf("Using configuration file: '%s'\n", config_file);

    FILE* config = fopen(config_file, "r");
    if(config == NULL) return 0;

    printf("Reading configuration file...\n");

    char buffer[300] = {0};
    while (fgets(buffer, sizeof(buffer), config)) {
        if(buffer[strlen(buffer) -1] == '\n') buffer[strlen(buffer) -1] = 0;
        li_append(other_nodes, buffer, strlen(buffer) + 1);
    }
    fclose(config);

    return 1;
}


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
    printf("Saving ip addresses to file '%s': \n", "node3.cfg");
    FILE* config = fopen("node3.cfg", "w"); //config_file
    if(config == NULL) return 0;

    li_foreach(other_nodes,li_write_string_file,config);

    fclose(config);

    return 0;

}




//Free all outstanding memory
void graceful_shutdown(int dummy) {
    printf("\nCommencing graceful shutdown!\n");

    //Write out new node list to file
    write_config();

    close_threads = 1;
    beaten = 2;

    //pthread_join(inbound_executor_thread, NULL);
    //pthread_join(outbound_network_thread, NULL);
    //pthread_join(inbound_network_thread, NULL);

    pthread_mutex_lock(&our_mutex);
    
    //Discard lists
    li_discard(other_nodes);
    li_discard(outbound_msg_queue);
    li_discard(inbound_msg_queue);

    //Discard blockchains
    discard_chain(our_chain);
    dict_foreach(foreign_chains, destroy_chains_in_dict, NULL);
    dict_discard(foreign_chains);
    

    //Discard keys
    destroy_keys(&our_keys, &pri_key, &pub_key);

    nn_close(sock_in);
    nn_close(sock_out);

    pthread_mutex_unlock(&our_mutex);

    pthread_mutex_destroy(&our_mutex);

    exit(0);
}



//The star of the show
int main(int argc, char* argv[]) {
    
    //Ctrl-C Handler
    signal(SIGINT, graceful_shutdown);

    //Create our blockchain
    our_chain = new_chain();
    beaten = 0;

    //Initialize Crypto
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    ERR_load_crypto_strings();

    //Get our ip address from argument
    if(argc < 2) strcpy(our_ip, "ipc:///tmp/pipeline_0.ipc");
    else if(strlen(argv[1]) < 300) strcpy(our_ip, argv[1]);
    else printf("Provide a valid address with a length less than 300 characters.\n");

    //Try to read keys first
    char pri_file[500];
    sprintf(pri_file, "pri.pem");
    char pub_file[500];
    sprintf(pub_file,"pub.pem");
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

    //Create foreign chain dict
    foreign_chains = dict_create();

    //Create list of other nodes
    other_nodes = list_create();


    //Get our config file from argument
    if(argc < 3) strcpy(config_file, "node.cfg");
    else if( argc == 3 && strlen(argv[2]) < 300) {
        strcpy(config_file, argv[2]);
    } 
    else{
        printf("Provide a valid config file path with a length less than 300 characters.\n");
        return 0;
    }
    int success = read_config();
    if(success) {
        printf("All good.\n");
    }
    else {
        printf("Please provide a proper configuration file.\n");
        return 0;
    }


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
    close_threads = 0;

    //Begin mining
    mine();

    return 0;

}
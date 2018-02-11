#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"
#include "data_containers/linked_list.h"
#include "data_containers/dict.h"
#include "blockchain.h"
#include "client.h"

//IDs
RSA* our_keys;
char* pub_key;
char* pri_key;
char asci_pub_key[500] = {0};

//Threads
pthread_t inbound_network_thread;
pthread_t outbound_network_thread;
pthread_mutex_t our_mutex;
int close_threads;

//Lists
list* other_nodes;
int number_of_nodes;
list* outbound_msg_queue; //holds outbound message structs
list* inbound_msg_queue; //holds recieved char* messages to execute

//Dicts
dict* posted_things;

//Sockets
int sock_in;
int sock_out;
char our_ip[300];
int last_check;

//Creates a sent object
p_thing* create_p_thing() {

    p_thing* temp = malloc(sizeof(p_thing));
    temp->amount = 0;
    temp->status = -1;
    temp->confirmed_by = dict_create();

    return temp;
}

//Destroys a sent object
void destroy_p_thing(p_thing* in_thing) {

    dict_discard(in_thing->confirmed_by);
    free(in_thing);
    return;
}


//Sets up a message
void setup_message(client_message_item* in_message) {
    in_message->status = -1;
    in_message->tries = 0;
    in_message->type = -1;
    memset(in_message->toWhom, 0, sizeof(in_message->toWhom));
    return;
}

//Display Help
void display_help() {
    printf("Help/Command List: 'h'\n");
    printf("Transactions List: 't'\n");
    printf("Submit a post: 'p note'\n");
    printf("Post New Transaction: 'n sender reciever amount'\n");
    printf("Quit program: 'q'\n");
    return;
}

//Exits the program
void quit_program() {
    printf("Shutting down. Good bye!\n");
    exit(0);
}

// Validates the transaction format
int val_trans_format(char* recipient, char* amount) {

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

//Gets the signature from the given post or transaction
int get_signature(char* output, char* input) {

    char our_message[5000];
    strcpy(our_message, input);

    int counter = 0;
    char* our_sig;
    char* temp = strtok(our_message," ");
    while( (temp = strtok(NULL, " ")) != NULL) {
        our_sig = temp;
        counter++;
    }
    if(counter < 4) return 0;
    strcpy(output,our_sig);
    return 1;

}

int register_new_node(char* input) {


    if(input == NULL) return 0;

    if(li_search(other_nodes, NULL, input, strlen(input) + 1) == NULL)
        number_of_nodes++;
 
    li_append(other_nodes,input, strlen(input) + 1);
    
    

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

    client_message_item announcement;
    setup_message(&announcement);

    //Get signature
    char output[513];
    int ret = get_signature(output,out_msg);

    strcpy(announcement.toWhom, data_string);
    strcpy(announcement.message, out_msg);
    
    if(ret != 0) {
        strcpy(announcement.sig, output);
        announcement.type = 1;
    }
    else {
        announcement.type = 0;
    }

    pthread_mutex_lock(&our_mutex);
    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
    pthread_mutex_unlock(&our_mutex);

    return NULL;
}

//Posts a post
void post_post(char* input) {

    if(input == NULL) return;

    char note[3] = {0};

    sscanf(input, "%*s %s", note);
    
    char out_msg[2000] = {0};
    char seperator = ' ';
    sprintf(out_msg,"P %ld%c%s%c%s%c", time(NULL),seperator, asci_pub_key, seperator, note, seperator);


    char sig[513] = {0};
    message_signature(sig,out_msg + 2,our_keys,pub_key);
    strcat(out_msg,sig);

    int status = -1;

    p_thing* new_post = create_p_thing();
    new_post->amount = 1;

    dict_insert(posted_things,sig,new_post, sizeof(*new_post));

    
    li_foreach(other_nodes,send_to_all, &out_msg);
    

    return;

}

//Posts a transaction
void post_transaction(char* input) {

    char recipient[32];
    char amount[32];

    sscanf(input, "%*s %s %s", recipient, amount);

    int the_amount = atoi(amount);
    sprintf(amount,"%010d", the_amount);
    
    if(!val_trans_format(recipient, amount))
        return;

    char out_msg[2000] = {0};
    char seperator = ' ';

    sprintf(out_msg,"T %ld%c%s%c%s%c%010d%c", time(NULL),seperator, asci_pub_key, seperator, recipient, seperator, the_amount, seperator);

    char sig[513] = {0};
    message_signature(sig,out_msg + 2, our_keys, pub_key);
    //strcat(out_msg, seperator);
    strcat(out_msg,sig);

    p_thing* new_trans = create_p_thing();
    new_trans->amount = the_amount;

    dict_insert(posted_things,sig,new_trans, sizeof(*new_trans));

    li_foreach(other_nodes,send_to_all, &out_msg);


    return;
}

//Read configuration file
int read_config2() {
    FILE* config = fopen("node.cfg", "r");
    if(config == NULL) return 0;

    number_of_nodes = 0;
    char buffer[120] = {0};
    while (fgets(buffer, sizeof(buffer), config)) {
        if(buffer[strlen(buffer) -1] == '\n') buffer[strlen(buffer) -1] = 0;
        li_append(other_nodes, buffer, strlen(buffer) + 1);
        number_of_nodes++;
    }
    printf("NUMBER OF NODES: %d\n", number_of_nodes);
    fclose(config);

    return 0;
}

//Done to each message in 'outbound_msg_queue'. input is of type client_message_item struct
void* process_outbound(list* in_list, li_node* input, void* data) {

    if(input == NULL) return NULL;

    client_message_item* our_message = (client_message_item*)input->data;

    sock_out = nn_socket(AF_SP, NN_PUSH);
    assert (sock_out >= 0);
    int timeout = 100;
    assert (nn_setsockopt(sock_out, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) >= 0);

    //printf("Sending to: %s: '%.10s'",our_message->toWhom, our_message->message);
    if(nn_connect (sock_out, our_message->toWhom) < 0){
        printf("Connection Error.\n");
        nn_close(sock_out);
    }
    int bytes = nn_send (sock_out, our_message->message, strlen(our_message->message), 0);
    //printf("Bytes sent: %d\n", bytes);
    usleep(100000);
    nn_close(sock_out);

    if(bytes > 0 || our_message->tries == 0){
        li_delete_node(in_list, input);

        if(our_message->type == 1) {
        int* current_status = &( ((p_thing*)dict_access(posted_things,our_message->sig))->status );

        if(*current_status == -1) {

            *current_status = 0;
        }
        }

    }
    else our_message->tries++;

    return NULL;
}

//Checks on each sent transaction and post whether they have been accepted or not. Keeps asking
int check_on_unaccepted(bt_node* current_node, void* data) {

    if(current_node == NULL) return 0;

    int value = ( ((p_thing*)current_node->data)->status );
    if( value == -1 || (value >= (number_of_nodes / 2.0)) ) return 1;

    char out_msg[1500];
    sprintf(out_msg,"V %s %s", our_ip, current_node->key);
    li_foreach(other_nodes,send_to_all, &out_msg);

    return 1;

}


//Outbound thread function - tried to send everything in outbound message queue
void* out_server() {
    last_check = time(NULL);
    while(true) {
        pthread_mutex_lock(&our_mutex);
        li_foreach(outbound_msg_queue, process_outbound, NULL);
        if(close_threads)
            return NULL;

        //Check on un verified messages

        if(time(NULL) - last_check > 10) {
            dict_foreach(posted_things,check_on_unaccepted, NULL);
            last_check = time(NULL);
        }      
        pthread_mutex_unlock(&our_mutex);
        sleep(1);
    }
}

//Print out posts and transactions
int print_posts_trans(bt_node* current_node, void* data) {

    int value = ( ((p_thing*)current_node->data)->status );

    printf("- '%.50s...'     %d     ",current_node->key, ((p_thing*)current_node->data)->amount );
    if( value == -1) printf("Unsent");
    else if( value == 0) printf("Sent");
    else if(value >= (number_of_nodes / 2.0)) printf("Accepted");

    if(value > -1) printf("    %d / %d nodes", value, number_of_nodes );
    printf("\n");

    return 1;
}

//Print the list of things that have been posted
void print_posted_items() {

    printf("SENT OUT:\n");
    printf("message & status\n");
    printf("-----------------------------------------\n");
    dict_foreach(posted_things, print_posts_trans, NULL);
    return;
}

//An incoming acceptance message coming in
int trans_or_post_acceptance( char* input) {

    char thing_to_accept[513] ={0};
    char foreign_node[300] ={0};

    sscanf(input,"%s %s", thing_to_accept, foreign_node);

    if(strlen(thing_to_accept) == 0 || strlen(foreign_node) == 0) return 0;

    p_thing* current_thing = (p_thing*)dict_access(posted_things, thing_to_accept);

    void* exists = dict_access(current_thing->confirmed_by,foreign_node);

    if(exists == NULL) {
        int a = 1;
        dict_insert(current_thing->confirmed_by,foreign_node,&a,sizeof(a));
        current_thing->status++;
    }

    return 1;
}

//Process the incoming message
void process_message(const char* in_msg) {


    if(in_msg == NULL) return;

    char to_process[30000] = {0};
    strcpy(to_process, in_msg);

    char* token = strtok(to_process," ");

    if(!strcmp(token, "V"))
        trans_or_post_acceptance(to_process + 2);
    else if(!strcmp(token, "N"))
        register_new_node(to_process + 2);

    return;

}

//Incoming Server
void* in_server() {

    sock_in = nn_socket (AF_SP, NN_PULL);
    assert (sock_in >= 0);
    assert (nn_bind (sock_in, our_ip) >= 0);
    int timeout = 500;
    assert (nn_setsockopt (sock_in, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof (timeout)) >= 0);

    char buf[30000];

    while(true) {
        int bytes = nn_recv(sock_in, buf, sizeof(buf), 0);
        
        pthread_mutex_lock(&our_mutex);
        if(bytes > 0) {
            buf[bytes] = 0;
            //printf("\nRecieved %d bytes: \"%s\"\n", bytes, buf);
            process_message(buf);
        }

        if(close_threads)
                return 0;
        pthread_mutex_unlock(&our_mutex);            

    }
    return 0;
}

//The moon of the show
int main(void) {
    
    //Setup
    printf("Blockchain in C: Client v1.0 by DG\n'h' for help/commandlist\n");
    char buffer[120] = {0};

    other_nodes = list_create();
    outbound_msg_queue = list_create();
    inbound_msg_queue = list_create();

    posted_things = dict_create();

    strcpy(our_ip, "ipc:///tmp/pipeline_001.ipc");


    //read_config();
    read_config2();

    //Create Keys
    //Try to read keys first
    char pri_file[500];
    sprintf(pri_file, "client_pri.pem");
    char pub_file[500];
    sprintf(pub_file,"client_pub.pem");
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
    strip_pub_key(asci_pub_key, pub_key);
    printf("%s%s\n\n", pri_key, pub_key);

    char buffer2[500];
    strcpy(buffer2, pub_key);


    //Threads
    pthread_mutex_t our_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_create(&outbound_network_thread, NULL, out_server, NULL);
    pthread_create(&inbound_network_thread, NULL, in_server, NULL);
    close_threads = 0;

    char ip_announcement[500];
    sprintf(ip_announcement,"I %s", our_ip);
    li_foreach(other_nodes,send_to_all, &ip_announcement);

    //Wait for command
    while(true) {
        printf("noincoin>");
        fgets(buffer, 120, stdin);
        buffer[strlen(buffer) - 1] = 0;

        if( !strcmp("h",buffer) || !strcmp("help",buffer) )
            display_help();

        else if( !strcmp("q", buffer) || !strcmp("quit", buffer) || !strcmp("exit", buffer) )
            quit_program();

        else if( !strcmp("t", buffer) )
            print_posted_items();
        else if(buffer[0] == 'n')
            post_transaction(buffer);
        else if(buffer[0] == 'p')
            post_post(buffer);
        else if( !strcmp("", buffer))
            ;
        else
            printf("Command not recognized.\n");

    }

    return 0;
}
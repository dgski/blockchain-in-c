#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "client_queue.h"
#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"
#include "linked_list_string.h"


transaction_queue*  main_queue;
pthread_t network_thread;
strlist* other_nodes;


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

bool val_trans_format(char* sender, char* recipient, char* amount) {

    for(int i = 0; i < strlen(sender); i++) {
        if(sender[i] == ':') {
            printf("You cannot use ':' in sender's or reciever's names.\n");
            return false;
        }
    }
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

void post_transaction(char* input) {

    char sender[32];
    char recipient[32];
    char amount[32];

    sscanf(input, "%*s %s %s %s", sender, recipient, amount);

    if(!val_trans_format(sender, recipient, amount))
        return;

    char out_msg[96];
    char seperator[] = " ";
    strcpy(out_msg, "T ");
    strcat(out_msg, sender);
    strcat(out_msg, seperator);
    strcat(out_msg, recipient);
    strcat(out_msg, seperator);
    strcat(out_msg, amount);

    c_transaction* temp = new_trans(0,sender, recipient, atoi(amount), out_msg);
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

int main(void) {
    
    //Setup
    printf("Blockchain in C: Client v0.1 by DG\n'h' for help/commandlist\n");
    char buffer[120] = {0};
    main_queue = new_queue();
    other_nodes = create_strlist();
    read_config();

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

        else if( !strcmp("", buffer))
            ;
        else
            printf("Command not recognized.\n");

    }

    return 0;
}
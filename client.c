#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h> 
#include "client_queue.h"


transaction_queue*  main_queue;
pthread_t network_thread;


void* send_new(c_transaction* in_trans) {

    if(in_trans->status == 0) {

        printf("Sending: %s\n", in_trans->message);
        in_trans->status = 1;
    }

    return NULL;
}


void* check_network(){
    
    while(true) {
        //1. Check for new transactions added to list
        //2. Send new transactions to nodes
        queue_map(main_queue, send_new);
        //3. Request blockchain status of transactions
        //4. Recieve new data
        //5. Update transactions status
        //6. Wait short amount to prevent spamming
        sleep(5);




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
    char seperator[] = ":";
    strcpy(out_msg, sender);
    strcat(out_msg, seperator);
    strcat(out_msg, recipient);
    strcat(out_msg, seperator);
    strcat(out_msg, amount);

    c_transaction* temp = new_trans(0,sender, recipient, atoi(amount), out_msg);
    add_to_queue(main_queue, temp);

    return;
}

void print_trans() {
    print_queue(main_queue);
    return;
}



int main(void) {

    //Setup
    printf("Blockchain in C: Client v0.1 by DG\n'h' for help/commandlist\n");
    char buffer[120] = {0};
    main_queue = new_queue();

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
            print_trans();

        else if(buffer[0] == 'n')
            post_transaction(buffer);

        else if( !strcmp("", buffer))
            ;
        else
            printf("Command not recognized.\n");

    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct c_transaction {
    unsigned int id;
    char sender[32];
    char recipient[32];
    int amount;
    char message[96 + 1];
    int status; // 0: Created, 1: Sent, 2: Posted
    struct c_transaction* next;
} c_transaction;

typedef struct transaction_queue {
    unsigned int amount;
    c_transaction* head;
} transaction_queue;

transaction_queue* new_queue() {
    transaction_queue* temp = malloc(sizeof(transaction_queue));
    temp->amount = 0;
    temp->head = NULL;

    return temp;
}

transaction_queue* add_to_queue(transaction_queue* in_queue, c_transaction* in_trans) {
    
    if(in_queue->head == NULL) {
        in_queue->head = in_trans;
    }
    else {
        c_transaction* temp = in_queue->head;
        while(temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = in_trans;
    }
    in_queue->amount++;
    return in_queue;
}

void print_queue(transaction_queue* in_queue) {

    printf("%d Transaction(s):\n\n", in_queue->amount);
    printf("id sender recipient amount message status\n");
    printf("-----------------------------------------\n");

    c_transaction* temp = in_queue->head;

    while(temp != NULL) {
        printf("%d %s %s %d '%s' %d\n",temp->id, temp->sender, temp->recipient, temp->amount, temp->message, temp->status);
        temp = temp->next;
    }
}

c_transaction* new_trans(int id, char* sender, char* recipient, int amount, char* message) {

    c_transaction* temp = malloc(sizeof(c_transaction));
    temp->id = id;
    strcpy(temp->sender, sender);
    strcpy(temp->recipient, recipient);
    temp->amount = amount;
    strcpy(temp->message, message);
    temp->status = 0;
    temp->next = NULL;

    return temp;
}

//Maps every transaction in list to a function
void queue_map(transaction_queue* in_queue, void* (*func)(c_transaction* input)) {
    c_transaction* temp = in_queue->head;
    while(temp != NULL) {
        (*func)(temp);
        temp = temp->next;
    }
    return;
}
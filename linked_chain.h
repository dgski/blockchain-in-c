#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "block.h"
#include <time.h>
#include "hashing.h"

typedef struct blink 
{
    block data;
    struct blink* next;
} blink;

unsigned char* hash_block(block* in_block);

void print_block(blink* in_block)
{
    printf("BLOCK # %d\n",in_block->data.index);
    printf("--------------------\n");
    printf("TIME: %d\n",in_block->data.time);
    printf("TRANSACTIONS:\n");
    
    for(int i = 0; i < in_block->data.trans_list_length; i++) {
        printf("%s : %s - %d\n", in_block->data.trans_list[i].sender, in_block->data.trans_list[i].recipient, in_block->data.trans_list[i].amount);
    }


    printf("PROOF: %d\n",in_block->data.proof);
    printf("PREV HASH: ");
    for(int i = 0; i < 32; i++) {
        printf("%d.",in_block->data.previous_hash[i]);
    }
    printf("\n\n");
}

blink* create()
{
    blink* temp = malloc(sizeof(blink));
    temp->data.index = 0;
    temp->data.time = time(NULL);
    temp->data.proof = 0;
    memset(temp->data.trans_list,0,sizeof(temp->data.trans_list));
    memcpy(temp->data.previous_hash,"AGENESISBLOCK", 32);
    temp->next = NULL;
    return temp;
}

blink* prepend(blink* head)
{
    blink* temp = create();
    temp->next = head;
    head = temp;
    return head;
}

blink* append(blink* head)
{
    blink* cursor = head;

    while(cursor->next != NULL)
        cursor = cursor->next;

    blink* temp = create();
    cursor->next = temp;
    
    return temp;
}

blink* remove_front(blink* head)
{
    blink* cursor = head;
    head = head->next;
    free(cursor);
    return head;
}

blink* remove_end(blink* head)
{
    blink* cursor = head;
    blink* back = NULL;    
    
    while(cursor->next != NULL)
    {
        back = cursor;
        cursor = cursor->next;
    }

    if(back != NULL)
        back->next = NULL;

    free(cursor);
    
    return head;
}

void print_list(blink* head)
{
    blink* temp = head;

    while(temp != NULL)
    {
        print_block(temp);
        temp = temp->next;
    }
    printf("\n");

}

void discard_list(blink* head)
{
    //Single element sized list
    if(head->next == NULL)
    {
        free(head);
        return;
    }

    blink* temp = head;
    temp = temp->next;

    while(head != NULL)
    {
        free(head);
        head = temp;
        if(temp != NULL)
            temp = temp->next;
    }
}

//Blockchain structure
typedef struct blockchain
{
    blink* head;
    unsigned char last_hash[32];
    transaction trans_list[20];
    unsigned int last_proof_of_work;
    int trans_index;
    int new_index;
    
} blockchain;

void setup_chain(blockchain* in_chain) {

    in_chain->head = create();
    in_chain->head->data.proof = 100;
    in_chain->last_proof_of_work = 100;
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    memcpy(in_chain->last_hash, hash_block(&in_chain->head->data),32);
    in_chain->trans_index = 0;
    in_chain->new_index = 1;

    return;
}


//Add transaction to transaction_list
void new_transaction(blockchain* in_chain, char* in_sender, char* in_recipient, int in_amount) {

    int index = in_chain->trans_index++;
    strcpy(in_chain->trans_list[index].sender, in_sender);
    strcpy(in_chain->trans_list[index].recipient, in_recipient);
    in_chain->trans_list[index].amount = in_amount;

    return;
}

//Create and append new block
blink* new_block(blockchain* in_chain, unsigned int in_proof) {

    //Create
    blink* new_block = append(in_chain->head);

    //Add data
    new_block->data.index = in_chain->new_index++;
    new_block->data.time = time(NULL);
    memcpy(new_block->data.trans_list,in_chain->trans_list, sizeof(in_chain->trans_list));
    new_block->data.trans_list_length= in_chain->trans_index;
    new_block->data.proof = in_proof;
    memcpy(new_block->data.previous_hash,in_chain->last_hash, 32);

    //Register new block hash
    memcpy(in_chain->last_hash, hash_block(&new_block->data), 32);

    //Reset blockchain intake
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    in_chain->last_proof_of_work = in_proof;
    in_chain->trans_index = 0;

    return new_block;
}

unsigned char* hash_block(block* in_block) {

    char block_string[1024];
    char buffer[120];

    //Add index and time
    sprintf(block_string,"%i%i", in_block->index, in_block->time);

    //Add transactions
    for(int i = 0; i < in_block->trans_list_length; i++) {

        sprintf(buffer,"%s%s%d", in_block->trans_list[i].sender,in_block->trans_list[i].recipient,in_block->trans_list[i].amount);
        strcat(block_string,buffer);
    }

    //Add transaction list length
    sprintf(buffer,"%d", in_block->trans_list_length);
    strcat(block_string, buffer);

    //Add proof
    sprintf(buffer,"%d", in_block->proof);
    strcat(block_string, buffer);

    //Add previous hash
    for(int i = 0; i < 32; i++) {
        sprintf(buffer,"%d", in_block->previous_hash[i]);
        strcat(block_string, buffer);
    }

    //char* hash_value = crypt(block_string,"BC");
    unsigned char* hash_value =  malloc(32);
    hash256(hash_value, block_string);

    return hash_value;
}

bool valid_proof(unsigned int last_proof, unsigned int proof) {

    char guess[120];
    sprintf(guess, "%i%i",last_proof, proof);
    unsigned char hash_value[32];
    hash256(hash_value,guess);
    return (hash_value[0] == '0' && hash_value[1] == '0' && hash_value[2] == '0' /*&& hash_value[3] == '0'*/);
    return false;
}

unsigned int proof_of_work(unsigned int last_proof) {

    unsigned int proof = 0;

    while(valid_proof(last_proof, proof) == false){
        //printf("%d\n", proof);
        proof += 1;
    }

    return proof;

}

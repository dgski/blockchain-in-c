#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "block.h"
#include <time.h>
#include <unistd.h>
#include <openssl/sha.ha>


typedef struct blink 
{
    block data;
    struct blink* next;
} blink;

char* hash_block(block* in_block);

blink* create()
{
    blink* temp = malloc(sizeof(blink));
    temp->data.index = 0;
    temp->data.time = time(NULL);
    temp->data.proof = 0;
    memset(temp->data.trans_list,0,sizeof(temp->data.trans_list));
    strcpy(temp->data.previous_hash,"AGENESISBLOCK");
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
        printf("BLOCK # %d\n",temp->data.index);
        printf("--------------------\n");
        printf("TIME: %d\n",temp->data.time);
        printf("TRANSACTIONS:\n");
        
        for(int i = 0; i < temp->data.trans_list_length; i++) {
            printf("%s : %s - %d\n", temp->data.trans_list[i].sender, temp->data.trans_list[i].recipient, temp->data.trans_list[i].amount);
        }


        printf("PROOF: %d\n",temp->data.proof);
        printf("PREV HASH: %s\n\n",temp->data.previous_hash);

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
    char last_hash[14];
    transaction trans_list[20];
    int trans_index;
    int new_index;
    
} blockchain;

void setup_chain(blockchain* in_chain) {

    in_chain->head = create();
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    strcpy(in_chain->last_hash, hash_block(&in_chain->head->data));
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
    strcpy(new_block->data.previous_hash,in_chain->last_hash);

    //Register new block hash
    strcpy(in_chain->last_hash, hash_block(&new_block->data));

    //Reset blockchain intake
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    in_chain->trans_index = 0;

    return new_block;
}

char* hash_block(block* in_block) {

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
    strcat(block_string, in_block->previous_hash);

    //char* hash_value = crypt(block_string,"BC");
    char* hash_value = SHA256()

    return hash_value;
}

bool valid_proof(int last_proof, int proof) {

    char guess[120];
    memset(guess, 0, sizeof(guess));
    sprintf(guess, "%i%i", last_proof,proof);

    char* hash_value = crypt(guess,"BC");

    printf("%s\n", hash_value);

    return (/*hash_value[0] == '0' && hash_value[1] == '0' &&*/ hash_value[2] == '0' && hash_value[3] == '0' && hash_value[4] == '0');

}

int proof_of_work(last_proof) {

    int proof = 0;

    while(valid_proof(last_proof, proof) == false)
        proof += 1;

    return proof;

}

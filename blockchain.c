#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "blockchain.h"
#include "hash.h"

//Create new Blockchain
blockchain* new_chain() {

    blockchain* in_chain = malloc(sizeof(blockchain));

    in_chain->head = blink_create();
    in_chain->head->data.time = 0;
    in_chain->head->data.proof = 100;
    memset(in_chain->head->data.trans_list,0, sizeof(in_chain->head->data.trans_list));

    in_chain->last_proof_of_work = 100;
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    memset(in_chain->new_posts, 0, sizeof(in_chain->new_posts));
    memcpy(in_chain->last_hash, hash_block(&in_chain->head->data),HASH_HEX_SIZE);
    
    in_chain->trans_index = 0;
    in_chain->new_index = 1;
    char block[BLOCK_STR_SIZE];
    string_block(block,&(in_chain->head->data));
    strcpy(in_chain->last_block,block);

    return in_chain;
}

//Add transaction to transaction_list
void new_transaction(blockchain* in_chain, char* in_sender, char* in_recipient, int in_amount, char* in_signature) {

    printf("NEW TRANSACTION!\n");
    printf("recipient: %s\n", in_recipient);
    //Transactions full

    if(in_chain->trans_index > 19)
        return;

    int index = in_chain->trans_index++;
    strcpy(in_chain->trans_list[index].sender, in_sender);
    strcpy(in_chain->trans_list[index].recipient, in_recipient);
    strcpy(in_chain->trans_list[index].signature, in_signature);
    in_chain->trans_list[index].amount = in_amount;

    //printf("TRANCTION DONE\n");

}

//Create and blink_append new block
blink* append_current_block(blockchain* in_chain, long in_proof) {

    //Create block
    blink* the_block = blink_append(in_chain->head);

    //Add data
    the_block->data.index = in_chain->new_index++;
    the_block->data.time = time(NULL);
    memcpy(the_block->data.trans_list,in_chain->trans_list, sizeof(in_chain->trans_list));
    memset(the_block->data.posts, 0, sizeof(the_block->data.posts));
    the_block->data.trans_list_length= in_chain->trans_index;
    the_block->data.proof = in_proof;
    memcpy(the_block->data.previous_hash,in_chain->last_hash, HASH_HEX_SIZE);

    //Register new block hash
    memcpy(in_chain->last_hash, hash_block(&the_block->data), HASH_HEX_SIZE);

    //Reset blockchain intake
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    in_chain->last_proof_of_work = in_proof;
    in_chain->trans_index = 0;

    //Register as latest block
    char block_str[BLOCK_STR_SIZE];
    string_block(block_str, &the_block->data);
    strcpy(in_chain->last_block,block_str);

    return the_block;
}

blink* append_new_block(blockchain* in_chain, unsigned int index, unsigned int in_time, transaction* trans_list,
 char* posts, unsigned int trans_list_length, long proof) {

     //Create block
    blink* the_block = blink_append(in_chain->head);

    //Add data
    the_block->data.index = index;
    the_block->data.time = in_time;
    memcpy(the_block->data.trans_list,trans_list,sizeof(the_block->data.trans_list));
    memset(the_block->data.posts, 0, sizeof(the_block->data.posts));
    the_block->data.trans_list_length= trans_list_length;
    the_block->data.proof = proof;
    memcpy(the_block->data.previous_hash,in_chain->last_hash, HASH_HEX_SIZE);

    //Register new block hash
    memcpy(in_chain->last_hash, hash_block(&the_block->data), HASH_HEX_SIZE);

    //Reset blockchain intake
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    in_chain->last_proof_of_work = proof;
    in_chain->trans_index = 0;
    in_chain->new_index++;

    //Register as latest block
    char block_str[BLOCK_STR_SIZE];
    string_block(block_str, &the_block->data);
    strcpy(in_chain->last_block,block_str);

    return the_block;



}

void print_block(blink* in_block, char separator)
{
    for(int i = 0; i < 77; i++)
        printf("%c", separator);
    printf("\n");
    
    printf("BLOCK # %d\n",in_block->data.index);
    printf("TIME: %d\n",in_block->data.time);
    printf("POSTS: %s\n",in_block->data.posts);
    printf("TRANSACTIONS:\n");

    for(int i = 0; i < in_block->data.trans_list_length; i++)
        printf("%.10s : %.10s - %d\n", in_block->data.trans_list[i].sender, in_block->data.trans_list[i].recipient, in_block->data.trans_list[i].amount);
    
    printf("PROOF: %ld\n",in_block->data.proof);
    printf("PREV HASH: ");
    printf("%s\n", in_block->data.previous_hash);
    /*
    for(int i = 0; i < 32; i++)
        printf("%02x",in_block->data.previous_hash[i]);
    printf("\n");
    */
    for(int i = 0; i < 77; i++)
        printf("%c", separator);
    printf("\n\n");
}

//Stringifies of the current block
char* string_block(char* output, block* in_block) {

    char block_string[BLOCK_STR_SIZE];
    char buffer[1100];

    //Add index and time
    sprintf(block_string,"%010i.%010i.", in_block->index, in_block->time);

    //Add posts
    strcat(block_string, in_block->posts);

    //Add transactions
    for(int i = 0; i < in_block->trans_list_length; i++) {

        sprintf(buffer,"%s:%s:%010d", in_block->trans_list[i].sender,in_block->trans_list[i].recipient,in_block->trans_list[i].amount);
        if(i + 1 != in_block->trans_list_length) strcat(buffer,"-");
        strcat(block_string,buffer);
    }
    strcat(block_string,".");

    //Add transaction list length
    sprintf(buffer,"%02d.", in_block->trans_list_length);
    strcat(block_string, buffer);

    //Add proof
    sprintf(buffer,"%020ld.", in_block->proof);
    strcat(block_string, buffer);

    //Add previous hash
    strcat(block_string, in_block->previous_hash);

    /*
    for(int i = 0; i < HASH_SIZE; i++) {
        sprintf(buffer,"%02x", in_block->previous_hash[i]);
        strcat(block_string, buffer);
    }*/

    strcpy(output, block_string);

    return output;
}



int extract_transactions(transaction* trans_array, char* in_trans) {
    
    char* trans_strings[20] = {0};

    char* pointer = strtok(in_trans,"-");
    trans_strings[0] = pointer;
    int i = 1;
    while(pointer != NULL) {
        pointer = strtok(NULL,"-");
        trans_strings[i++] = pointer;
    }
    /*
    for(int i = 0; trans_strings[i] != 0; i++) {
        printf("TRANSACTION: %s\n", trans_strings[i]);
    }*/

    char* sender;
    char* reciever;
    char* amount;

    for(int i = 0; trans_strings[i] != 0; i++) {
        sender = strtok(trans_strings[i],":");
        //printf("sender: %s\n", sender);
        reciever = strtok(NULL, ":");
        //printf("reciever: %s\n", reciever);
        amount = strtok(NULL, ":");
        //printf("amount: %s\n", amount);
        strcpy(trans_array[i].sender, sender);
        strcpy(trans_array[i].recipient, reciever);
        trans_array[i].amount = atoi(amount);
    }

    return 0;
}

char* hash_block(block* in_block) {

    char block_string[BLOCK_STR_SIZE];
    string_block(block_string, in_block);

    unsigned char* hash_value =  malloc(HASH_SIZE);
    hash256(hash_value, block_string);

    char* hash_hex = malloc(HASH_HEX_SIZE);
    memset(hash_hex, 0, HASH_HEX_SIZE);

    char buffer[3];
    for(int i = 0; i < HASH_SIZE; i++) {
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer,"%02x", hash_value[i]);
        strcat(hash_hex, buffer);
    }

    free(hash_value);


    return hash_hex;
}

bool valid_proof(char* last_hash, long proof) {

    char guess[GUESS_SIZE] = {0};
    sprintf(guess, "%s%020ld",last_hash, proof);

    unsigned char hash_value[HASH_SIZE];
    hash256(hash_value,guess);

    return (hash_value[0] == '0' && hash_value[1] == '0' && hash_value[2] == '0' && (hash_value[3] > 60 && hash_value[3] < 127));
}

long proof_of_work(int* beaten, char* last_hash) {

    long proof = 0;

    while(valid_proof(last_hash, proof) == false){
        //printf("%d\n", proof);
        proof += 1;
        
        if(*beaten) {
            return -1;
        }
    }
    /*
    printf("OUR PROOF: %ld\n", proof);
    printf("CONFIRMATION: %d\n", valid_proof(last_hash, proof));

    char guess[GUESS_SIZE] = {0};
    sprintf(guess, "%s%020ld",last_hash, proof);*/

    return proof;
}


//Block-link FUNCTIONS:
blink* blink_create()
{
    blink* temp = malloc(sizeof(blink));
    temp->data.index = 0;
    temp->data.time = time(NULL);
    temp->data.proof = 0;
    memset(temp->data.trans_list,0,sizeof(temp->data.trans_list));
    memcpy(temp->data.previous_hash,"AGENESISBLOCK", HASH_HEX_SIZE);
    temp->next = NULL;
    return temp;
}

blink* blink_prepend(blink* head)
{
    if(head == NULL)
        return head;

    blink* temp = blink_create();
    temp->next = head;
    head = temp;
    return head;
}

blink* blink_append(blink* head)
{
    blink* cursor = head;

    while(cursor->next != NULL)
        cursor = cursor->next;

    blink* temp = blink_create();
    cursor->next = temp;
    
    return temp;
}

blink* blink_remove_front(blink* head)
{
    blink* cursor = head;
    head = head->next;
    free(cursor);
    return head;
}

blink* blink_remove_end(blink* head)
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

void blink_print_list(blink* head)
{
    blink* temp = head;

    while(temp != NULL)
    {
        print_block(temp,'-');
        temp = temp->next;
    }
    printf("\n");

}

void blink_discard_list(blink* head)
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






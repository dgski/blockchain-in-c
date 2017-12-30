#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "block.h"
#include <time.h>

typedef struct link 
{
    int value;
    block data;
    struct link* next;
} link;

link* create(int input_value)
{
    link* temp = malloc(sizeof(link));
    temp->value = input_value;
    temp->next = NULL;
    return temp;
}

link* prepend(link* head, int input_value)
{
    link* temp = create(input_value);
    temp->next = head;
    head = temp;
    return head;
}

link* append(link* head, int input_value)
{
    link* cursor = head;

    while(cursor->next != NULL)
        cursor = cursor->next;

    link* temp = create(input_value);
    cursor->next = temp;
    
    return head;
}

link* remove_front(link* head)
{
    link* cursor = head;
    head = head->next;
    free(cursor);
    return head;
}

link* remove_end(link* head)
{
    link* cursor = head;
    link* back = NULL;    
    
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

void print_list(link* head)
{
    link* temp = head;

    while(temp != NULL)
    {
        printf("BLOCK # %d\n",temp->data.index);
        printf("--------------------\n");
        printf("TIME: %d\n",temp->data.time);
        printf("TRANSLIST_LENGTH: %lu\n",sizeof(temp->data.trans_list));
        printf("PROOF: %d\n",temp->data.proof);
        printf("PREV HASH: %s\n\n",temp->data.previous_hash);

        temp = temp->next;
    }
    printf("\n");

}

void discard_list(link* head)
{
    //Single element sized list
    if(head->next == NULL)
    {
        free(head);
        return;
    }

    link* temp = head;
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
    link* head;
    int new_index;
    
} blockchain;

void setup_chain(blockchain* in_chain) {

    in_chain->head = create(10);
    in_chain->new_index = 1;

    return;
}

//Create and append new block
link* new_block(blockchain* in_chain, unsigned int in_proof, char * in_previous_hash) {

    //Create
    link* new_block = append(in_chain->head, 100);

    //Add data
    new_block->data.index = in_chain->new_index++;
    new_block->data.time = time(NULL);
    memcpy(new_block->data.trans_list,new_block->data.trans_list, sizeof(new_block->data.trans_list));
    new_block->data.proof = in_proof;
    strcpy(new_block->data.previous_hash,in_previous_hash);



    return new_block;
}
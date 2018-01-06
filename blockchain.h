#include <stdbool.h>

#define BLOCK_STR_SIZE 1024
#define BLOCK_DATA_SIZE 10

//Transaction structure
typedef struct transaction {
    char sender[32];
    char recipient[32];
    int amount;
} transaction;

//Block Structure
typedef struct block {
    unsigned int index;
    unsigned int time;
    transaction trans_list[20];
    char posts[BLOCK_DATA_SIZE];
    unsigned int trans_list_length;
    unsigned int proof;
    unsigned char previous_hash[32];
} block;

//Chain link structure
typedef struct blink {
    block data;
    struct blink* next;
} blink;

//Blockchain structure
typedef struct blockchain {
    blink* head;
    char last_block[BLOCK_STR_SIZE];
    unsigned char last_hash[32];
    transaction trans_list[20];
    char new_posts[BLOCK_DATA_SIZE];
    unsigned int last_proof_of_work;
    int trans_index;
    int new_index;
} blockchain;

//Chain functions
blockchain* new_chain();
void new_transaction(blockchain* in_chain, char* in_sender, char* in_recipient, int in_amount);
blink* new_block(blockchain* in_chain, unsigned int in_proof);

//Block functions
void print_block(blink* in_block);
char* string_block(char* output, block* in_block);
unsigned char* hash_block(block* in_block);

//Link functions
blink* blink_create();
blink* blink_prepend(blink* head);
blink* blink_append(blink* head);
blink* blink_remove_front(blink* head);
blink* blink_remove_end(blink* head);
void blink_print_list(blink* head);
void blink_discard_list(blink* head);

//Work functions
bool valid_proof(unsigned char* last_hash, unsigned int proof);
unsigned int proof_of_work(int* beaten, unsigned char* last_hash);





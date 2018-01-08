#include <stdbool.h>

#define BLOCK_STR_SIZE 1572
#define BLOCK_DATA_SIZE 8
#define TRANS_LIST_SIZE 20
#define HASH_SIZE 32
#define USER_ADDRESS_SIZE 32
#define GUESS_SIZE 200

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_CYAN    "\x1b[36m"




//Transaction structure
typedef struct transaction {
    char sender[USER_ADDRESS_SIZE];
    char recipient[USER_ADDRESS_SIZE];
    int amount;
} transaction;

//Block Structure
typedef struct block {
    unsigned int index;
    unsigned int time;
    transaction trans_list[TRANS_LIST_SIZE];
    char posts[BLOCK_DATA_SIZE];
    unsigned int trans_list_length;
    long proof;
    unsigned char previous_hash[HASH_SIZE];
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
    unsigned char last_hash[HASH_SIZE];
    transaction trans_list[TRANS_LIST_SIZE];
    char new_posts[BLOCK_DATA_SIZE];
    long last_proof_of_work;
    int trans_index;
    int new_index;
} blockchain;

//Chain functions
blockchain* new_chain();
void new_transaction(blockchain* in_chain, char* in_sender, char* in_recipient, int in_amount);
blink* append_current_block(blockchain* in_chain, long in_proof);
blink* append_new_block(blockchain* in_chain, unsigned int index, unsigned int in_time, transaction* trans_list,
 char* posts, unsigned int trans_list_length, long proof);

//Block functions
void print_block(blink* in_block, char separator);
char* string_block(char* output, block* in_block);
unsigned char* hash_block(block* in_block);
int extract_transactions(transaction* trans_array, char* in_trans);

//Link functions
blink* blink_create();
blink* blink_prepend(blink* head);
blink* blink_append(blink* head);
blink* blink_remove_front(blink* head);
blink* blink_remove_end(blink* head);
void blink_print_list(blink* head);
void blink_discard_list(blink* head);

//Work functions
bool valid_proof(unsigned char* last_hash, long proof);
long proof_of_work(int* beaten, unsigned char* last_hash);





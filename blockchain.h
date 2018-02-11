#include <stdbool.h>
#include "data_containers/dict.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#define BLOCK_STR_SIZE 30000
#define BLOCK_DATA_SIZE 8
#define BLOCK_BUFFER_SIZE 5000
#define TRANS_LIST_SIZE 20
#define TRANS_STRING_LENGTH 3000
#define HASH_SIZE 32
#define HASH_HEX_SIZE 65
#define PUBLIC_ADDRESS_SIZE 500
#define SIG_SIZE 513
#define GUESS_SIZE 200
#define CURRENCY_CAP 888
#define CURRENCY_SPEED 2

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"

/////////////////////////////////////////////
// STRUCTURES
/////////////////////////////////////////////

//Transaction structure
typedef struct transaction {
    int time_of;
    char sender[PUBLIC_ADDRESS_SIZE];
    char recipient[PUBLIC_ADDRESS_SIZE];
    int amount;
    char signature[550];
} transaction;

//Post structure
typedef struct post {
    int time_of;
    char poster[PUBLIC_ADDRESS_SIZE];
    char data;
    char signature[550];
} post;

//Block Structure
typedef struct block {
    unsigned int index;
    unsigned int time;
    transaction trans_list[TRANS_LIST_SIZE];
    post posts[BLOCK_DATA_SIZE];
    unsigned int posts_list_length;
    unsigned int trans_list_length;
    long proof;
    char previous_hash[HASH_HEX_SIZE];
} block;

//Link in a blockchain
typedef struct blink {
    block data;
    struct blink* next;
} blink;

//Blockchain structure
typedef struct blockchain {
    blink* head;
    char last_block[BLOCK_STR_SIZE];
    char last_hash[HASH_HEX_SIZE];

    transaction trans_list[TRANS_LIST_SIZE];

    post new_posts[BLOCK_DATA_SIZE];
    
    char trans_hash[HASH_HEX_SIZE];

    long last_proof_of_work;
    int trans_index;
    int post_index;
    unsigned int length;
    unsigned int total_currency;

    dict* quickledger;
    dict* verified;

} blockchain;

//Alternate Chain structure
typedef struct alt_chain {
    blockchain* the_chain;
    int expected_index;
    int expected_length;
    unsigned int last_time;
} alt_chain;


/////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////

//Chain functions
blockchain* new_chain();
int discard_chain(blockchain* in_chain);
void new_transaction(blockchain* in_chain, int time_of, char* in_sender, char* in_recipient, int in_amount, char* in_signature);
void new_post(blockchain* in_chain, int time_of, char* in_sender, char in_data, char* in_signature);
blink* append_current_block(blockchain* in_chain, long in_proof);
blink* append_new_block(blockchain* in_chain, unsigned int index, unsigned int in_time, transaction* trans_list,
 post* posts, unsigned int trans_list_length, unsigned int posts_list_length, long proof);

//Block functions
void print_block(blink* in_block, char separator);
char* string_block(char* output, block* in_block);
char* hash_block(char* input, block* in_block);
int extract_transactions(blockchain* in_chain,transaction* trans_array, const char* in_trans);
int extract_transactions_raw(transaction* trans_array, char* in_trans);

int extract_posts_raw(post* post_array, char* input_posts_string);
int validate_posts(blockchain* in_chain, post* new_post_array, int nr_of_posts);

//Transaction functions
char* string_trans_nosig(char* output, int time_of, char* sender, char* reciever, int amount);
char* string_post_nosig(char* output, int time_of, char* sender, char data);
int strip_pub_key(char* output, char* input);

//Link functions
blink* blink_create();
blink* blink_prepend(blink* head);
blink* blink_append(blink* head);
blink* blink_remove_front(blink* head);
blink* blink_remove_end(blink* head);
void blink_print_list(blink* head);
void blink_discard_list(blink* head);

//Work functions
bool valid_proof(char* last_hash, char* trans_hash,  long proof);
long proof_of_work(int* beaten, char* last_hash, char* trans_hash);

//Crypto functions
void hash256(unsigned char* output, const char* input);
int create_keys(RSA** your_keys, char** pri_key, char** pub_key);
int destroy_keys(RSA** your_keys, char** pri_key, char** pub_key);
int read_keys(RSA** our_keys, char* pri_filename, char* pub_filename);
int write_keys(RSA** our_keys, char* pri_filename, char* pub_filename);
int message_signature(char* output, char* message, RSA* keypair, char* pub_key);
bool verify_message(const char* input, char* sender, char* signature);
int hash_transactions(char* output, transaction* trans_array, unsigned int trans_array_length, post* post_array, unsigned int post_array_length);







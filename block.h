//Transaction structure
typedef struct transaction
{
    char sender[32];
    char recipient[32];
    int amount;
} transaction;

//Block Structure
typedef struct block
{
    unsigned int index;
    unsigned int time;
    transaction trans_list[20];
    unsigned int trans_list_length;
    unsigned int proof;
    unsigned char previous_hash[32];
} block;


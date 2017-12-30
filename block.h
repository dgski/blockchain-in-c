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
    unsigned int trans_list_end;
    unsigned int proof;
    char previous_hash[100];

} block;

//Add transaction to transaction_list
int new_transaction(block* current_block, char* in_sender, char* in_recipient, int in_amount) {

    int index = ++current_block->trans_list_end;
    strcpy(current_block->trans_list[index].sender, in_sender);
    strcpy(current_block->trans_list[index].recipient, in_recipient);
    current_block->trans_list[index].amount = in_amount;

    return 0;
}
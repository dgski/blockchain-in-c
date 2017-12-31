#include <string.h>
#include "linked_chain.h"

//Main function
int main(void) {

    //Create blockchain
    blockchain our_chain;
    setup_chain(&our_chain);

    //Test transactions
    new_transaction(&our_chain,"David","Paul",100);
    new_transaction(&our_chain,"Richie","Frodo",200);
    new_transaction(&our_chain,"Gandalf","Frodo",500);

    //Test block
    blink* a_block = new_block(&our_chain,10);
    hash_block(&a_block->data);

    //Test transactions
    new_transaction(&our_chain,"BillGates","Steven",100);
    new_transaction(&our_chain,"BartSimpson","NinjaTurtle",200);

    //Test block
    a_block = new_block(&our_chain,100);
    hash_block(&a_block->data);


    print_list(our_chain.head);


    //proof_of_work(200);

    return 0;

}
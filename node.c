#include <string.h>
#include "linked_chain.h"

//Main function
int main(void) {

    //Create blockchain
    blockchain our_chain;
    setup_chain(&our_chain);

    //Test block
    new_block(&our_chain,10, "HellO");
    new_block(&our_chain,12, "HellO");
    new_block(&our_chain,13, "HellO");


    print_list(our_chain.head);


}
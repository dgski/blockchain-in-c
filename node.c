#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

#include "blockchain.h"
#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"

char node_name[60];
blockchain* our_chain;
pthread_t network_thread;


int mine() {
    printf("Mining started.\n");

    unsigned int result;
    
    while(true) {
        result = proof_of_work(our_chain->last_proof_of_work);

        if(result) {
            our_chain->last_proof_of_work = result;
            new_transaction(our_chain,"node","node", 100);
            blink* a_block = new_block(our_chain, our_chain->last_proof_of_work);
            print_block(a_block);
        }
    }
}



int insert_trans(char* input) {

    char sender[32] = {0};
    char recipient[32] = {0};
    int amount;

    sscanf(input, "%s %s %d", sender, recipient, &amount);
    new_transaction(our_chain,sender,recipient,amount);

    return 0;
}


void* server() {

    printf("Blockchain in C: Server v0.1\n");
    printf("Node name: %s\n", node_name);

    int sock = nn_socket (AF_SP, NN_PULL);
    assert (sock >= 0);
    assert (nn_bind (sock, "ipc:///tmp/pipeline.ipc") >= 0);

    while(true) {

        char buf[100] = {0};
        int bytes = nn_recv (sock, buf, sizeof(buf), 0);

        if(bytes > 0) {
            printf ("b-in-c-server: RECEIVED \"%s\"\n", buf);
            insert_trans(buf);
            //print_list(our_chain->head);
        }

    }

    return 0;
}




//Main function
int main(void) {
    
    //Create blockchain
    our_chain = new_chain();

    //Generate random node name
    strcpy(node_name, "best_node");

    pthread_create(&network_thread, NULL, server, NULL);

    mine();
    


    /*
    //Create blockchain
    our_chain = new_chain();

    //Test transactions
    new_transaction(our_chain,"David","Paul",100);
    new_transaction(our_chain,"Richie","Frodo",200);
    new_transaction(our_chain,"Gandalf","Frodo",500);

    //Test block
    blink* a_block = new_block(our_chain, proof_of_work(our_chain->last_proof_of_work));
    hash_block(&a_block->data);

    //Test transactions
    new_transaction(our_chain,"BillGates","Steven",100);
    new_transaction(our_chain,"BartSimpson","NinjaTurtle",200);

    //Test block
    a_block = new_block(our_chain, proof_of_work(our_chain->last_proof_of_work));
    hash_block(&a_block->data);


    print_list(our_chain->head);*/

    return 0;

}
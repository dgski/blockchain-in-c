#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"

#include "blockchain.h"
#include "linkedliststring.h"


//Global variables
char node_name[60];
unsigned int node_earnings;
strlist* other_nodes;
blockchain* our_chain;
pthread_t network_thread;

int announce_block(block* in_block) {

    char block[BLOCK_STR_SIZE];
    string_block(block, in_block);
    printf("%s\n", block);
    /*
    int sock = nn_socket (AF_SP, NN_PUSH);

    for(int i = 0; i < other_nodes->length; i++) {




    }*/


    return 0;
}



//Continually searches for proper proof of work
int mine() {

    printf("Mining started.\n");
    unsigned int result;
    
    while(true) {
        unsigned int time_1 = time(NULL);
        result = proof_of_work(our_chain->last_proof_of_work);
        unsigned int time_2 = time(NULL);

        printf("PROOF OF WORK FOUND: %d\n", result);
        printf("Time taken to mine: %f\n", (time_2 - time_1)/60.0);

        if(result) {
            our_chain->last_proof_of_work = result;
            new_transaction(our_chain,node_name,node_name, 100);
            node_earnings += 100;
            blink* a_block = new_block(our_chain, our_chain->last_proof_of_work);
            print_block(a_block);
            printf("TOTAL NODE EARNINGS: %d", node_earnings);
        }
    }

}


//Insert transaction
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
    //int timeo = 200;
    //assert (nn_setsockopt (sock, NN_SOL_SOCKET, NN_RCVTIMEO,&timeo, sizeof (timeo)));

    char buf[100] = {0};

    while(true) {

        memset(buf, 0, sizeof(buf));

        int bytes = nn_recv (sock, buf, sizeof(buf), 0);
        sleep(1);

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
    srand(time(NULL));   // should only be called once
    int r = rand();   
    sprintf(node_name, "node%d", r);

    //Create list of other nodes
    other_nodes = create_strlist();
    
    strli_append(other_nodes, "node1023984384");
    strli_append(other_nodes, "node101eoefjejfe232f23f");
    strli_append(other_nodes, "nodehelale");
    strli_print(other_nodes);
    strli_discard(other_nodes);

    //Reset node earnings
    node_earnings = 0;

    //Listen and respond to network connections
    pthread_create(&network_thread, NULL, server, NULL);

    //Begin mining
    mine();

    return 0;

}
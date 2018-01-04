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
int* beaten;
strlist* other_nodes;
blockchain* our_chain;
pthread_t network_thread;



void* announce_block(strli_node* in_item) {

    int sock = nn_socket (AF_SP, NN_PUSH);
    assert (sock >= 0);
    int timeout = 200;
    assert (nn_setsockopt(sock, NN_PUSH, NN_SNDTIMEO, &timeout,sizeof(timeout)));
    assert (nn_connect (sock, in_item->value) >= 0);
    printf("Announcing to: %s, ", in_item->value);
    int bytes = nn_send (sock, our_chain->last_block, strlen(our_chain->last_block), NN_DONTWAIT);
    printf("Bytes sent: %d\n", bytes);
    nn_shutdown(sock,0);

    return NULL;
}



//Continually searches for proper proof of work
int mine() {
    sleep(1);
    printf("Mining started.\n");
    unsigned int result;
    
    while(true) {
        unsigned int time_1 = time(NULL);
        result = proof_of_work(beaten, our_chain->last_hash);
        unsigned int time_2 = time(NULL);

        //printf("PROOF OF WORK FOUND: %d\n", result);
        printf("\nMINED: %f mins\n", (time_2 - time_1)/60.0);

        if(result) {
            our_chain->last_proof_of_work = result;
            new_transaction(our_chain,node_name,node_name, 100);
            node_earnings += 100;

            blink* a_block = new_block(our_chain, our_chain->last_proof_of_work);
            //memcpy(our_chain->last_block, &(a_block->data),sizeof(block));
            
            print_block(a_block);
            strli_map(other_nodes,announce_block);
            printf("\nTOTAL NODE EARNINGS: %d notes\n", node_earnings);
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
    printf("Node name: %s\n\n", node_name);

    int sock_in = nn_socket (AF_SP, NN_PULL);
    assert (sock_in >= 0);
    assert (nn_bind (sock_in, "ipc:///tmp/pipeline.ipc") >= 0);
    int timeout = 200;
    assert (nn_setsockopt (sock_in, NN_PULL, NN_RCVTIMEO,&timeout, sizeof (timeout)));

    char buf[100] = {0};

    while(true) {

        //Receive
        memset(buf, 0, sizeof(buf));
        int bytes = nn_recv (sock_in, buf, sizeof(buf), 0);
        sleep(1);
        if(bytes > 0) {
            printf ("RECEIVED \"%s\"\n", buf);
            insert_trans(buf);
            //print_list(our_chain->head);
        }

        //Send

    }

    return 0;
}




//Main function
int main(void) {
    
    //Create blockchain
    our_chain = new_chain();

    //Beaten variable
    beaten = malloc(sizeof(int));
    *beaten = 0;

    //Generate random node name
    srand(time(NULL));   // should only be called once
    int r = rand();   
    sprintf(node_name, "node%d", r);

    //Create list of other nodes
    other_nodes = create_strlist();
    
    strli_append(other_nodes, "ipc:///tmp/pipeline_1.ipc");
    strli_append(other_nodes, "ipc:///tmp/pipeline_2.ipc");
    //strli_print(other_nodes);

    //Reset node earnings
    node_earnings = 0;

    //Listen and respond to network connections
    pthread_create(&network_thread, NULL, server, NULL);

    //Begin mining
    mine();

    return 0;

}
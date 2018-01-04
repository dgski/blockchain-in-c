#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"

#include "blockchain.h"
#include "linkedliststring.h"

//Global variables
char node_name[60];
unsigned int node_earnings;
int* beaten;
blockchain* our_chain;

pthread_t network_thread;
char our_ip[100] = {0};
strlist* outbound_msgs;
strlist* other_nodes;


//Send new block block to other nodes
void* announce_block(strli_node* in_item) {

    int sock = nn_socket (AF_SP, NN_PUSH);
    assert (sock >= 0);
    int timeout = 200;
    assert (nn_setsockopt(sock, NN_PUSH, NN_SNDTIMEO, &timeout,sizeof(timeout)));
    assert (nn_connect (sock, in_item->value) >= 0);
    printf("Announcing to: %s, \n", in_item->value);
    int bytes = nn_send (sock, our_chain->last_block, strlen(our_chain->last_block), 0);
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
            new_transaction(our_chain,node_name,node_name, 10);
            node_earnings += 10;

            blink* a_block = new_block(our_chain, our_chain->last_proof_of_work);
            
            print_block(a_block);
            strli_foreach(other_nodes,announce_block);
            printf("\nTOTAL NODE EARNINGS: %d notes\n", node_earnings);
        }
    }
}


//Insert transaction [sender receiver amount]
int insert_trans(char* input) {

    printf("Inserting Transaction!\n");
    char sender[32] = {0};
    char recipient[32] = {0};
    int amount;
    sscanf(input, "%s %s %d", sender, recipient, &amount);
    new_transaction(our_chain,sender,recipient,amount);
    return 0;
}

//Insert music [sender music]
int insert_music(char* input) {

    printf("Inserting Music!\n");
    char sender[32] = {0};
    char music[64] = {0};
    sscanf(input, "%s %s", sender, music);
    return 0;
}
//Verify Foreign Block []
void verify_foreign_block(char* input) {

    printf("Verifying Foreign Block!\n");


}

//Regster New Node [node_ip]
void register_new_node(char* input) {
    
    printf("Regstering New Block:");
    if(strli_search(other_nodes, NULL, input))
        return;

    strli_append(other_nodes, input);
}


//Message type: T: transaction, M: music, B: block
void process_message(char* in_msg) {

    char* token = strtok(in_msg," ");
    printf("MESSAGE TYPE: %s\n", token);

    printf("MESSAGE CONTENTS: %s\n", in_msg + 2);

    if(!strcmp(token, "T"))
        insert_trans(in_msg + 2);
    if(!strcmp(token, "M"))
        insert_music(in_msg + 2);
    if(!strcmp(token, "B"))
        verify_foreign_block(in_msg + 2);
    if(!strcmp(token, "N"))
        register_new_node(in_msg + 2);


}

void* sendmsg(strli_node* item) {

    printf("SENDING: %s", item->value);
    char* toWhom = strtok(item->value,":::");
    char* out_msg = strtok(NULL, ":::");

    printf("toWHom: %s\n", toWhom);
    printf("out_msg: %s\n", out_msg);

    /*
    int sock = nn_socket (AF_SP, NN_PUSH);
    assert (sock >= 0);
    int timeout = 200;
    assert (nn_setsockopt(sock, NN_PUSH, NN_SNDTIMEO, &timeout,sizeof(timeout)));
    assert (nn_connect (sock, in_item->value) >= 0);
    printf("Announcing to: %s, ", in_item->value);
    int bytes = nn_send (sock, our_chain->last_block, strlen(our_chain->last_block), 0);
    printf("Bytes sent: %d\n", bytes);
    nn_shutdown(sock,0);*/


    return NULL;
}


//Network interface function
void* server() {

    printf("Blockchain in C Major: Server v0.1\n");
    printf("Node name: %s\n\n", node_name);

    int sock_in = nn_socket (AF_SP, NN_PULL);
    assert (sock_in >= 0);
    assert (nn_bind (sock_in, our_ip) >= 0);
    int timeout = 200;
    assert (nn_setsockopt (sock_in, NN_PULL, NN_RCVTIMEO,&timeout, sizeof (timeout)));

    char buf[100] = {0};

    while(true) {

        //Receive
        memset(buf, 0, sizeof(buf));
        int bytes = nn_recv (sock_in, buf, sizeof(buf), 0);
        if(bytes > 0) {
            printf("RECEIVED \"%s\"\n", buf);
            process_message(buf);
        }
        sleep(1);

        //Send
        /*strli_foreach(outbound_msgs, sendmsg);*/

    }

    return 0;
}




//Main function
int main(int argc, char* argv[]) {
    
    //Create blockchain
    our_chain = new_chain();

    //Beaten variable
    beaten = malloc(sizeof(int));
    *beaten = 0;

    //Generate random node name
    srand(time(NULL));   // should only be called once
    int r = rand();   
    sprintf(node_name, "node%d", r);

    //Get our ip address from argv
    if(argc < 2)
        strcpy(our_ip, "ipc:///tmp/pipeline.ipc");
    else
        strcpy(our_ip, argv[1]);
    
    /*
    //Create outbound message list & add our IP to be sent to all nodes
    strlist* outbound_msgs = create_strlist();
    char ip_broadcast[120] = {0};
    strcpy(ip_broadcast, "all:::N ");
    strcat(ip_broadcast, our_ip);
    strli_append(outbound_msgs, ip_broadcast);
    */

    //Create list of other nodes
    other_nodes = create_strlist();
    strli_append(other_nodes, "ipc:///tmp/pipeline_1.ipc");
    strli_append(other_nodes, "ipc:///tmp/pipeline_2.ipc");
    strli_print(other_nodes);


    //Reset node earnings
    node_earnings = 0;

    //Listen and respond to network connections
    pthread_create(&network_thread, NULL, server, NULL);

    //Begin mining
    mine();

    return 0;

}
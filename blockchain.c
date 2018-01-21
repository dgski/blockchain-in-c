#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <signal.h>

#include "blockchain.h"
#include "hash.h"


//Create new Blockchain
blockchain* new_chain() {

    blockchain* in_chain = malloc(sizeof(blockchain));

    in_chain->head = blink_create();
    in_chain->head->data.time = 0;
    in_chain->head->data.proof = 100;
    in_chain->head->data.trans_list_length = 0;
    memset(in_chain->head->data.trans_list,0, sizeof(in_chain->head->data.trans_list));
    memset(in_chain->head->data.posts,0, sizeof(in_chain->head->data.posts));


    in_chain->last_proof_of_work = 100;
    memset(in_chain->trans_list, 0, sizeof(in_chain->trans_list));
    memset(in_chain->new_posts, 0, sizeof(in_chain->new_posts));
    memcpy(in_chain->last_hash, hash_block(&in_chain->head->data),HASH_HEX_SIZE);
    
    in_chain->trans_index = 0;
    in_chain->length = 0;

    in_chain->quickledger = dict_create();

    in_chain->total_currency = 0;
    
    char block[BLOCK_STR_SIZE];
    string_block(block,&(in_chain->head->data));
    strcpy(in_chain->last_block,block);



    return in_chain;
}

int discard_chain(blockchain* in_chain) {

    if(in_chain == NULL) return 0;

    //Discard list of blocks in chain
    blink_discard_list(in_chain->head);

    //Discard quickledger
    dict_discard(in_chain->quickledger);

    //Free memory in struct
    free(in_chain);

    return 1;
}

//Add transaction to transaction_list
void new_transaction(blockchain* in_chain, char* in_sender, char* in_recipient, int in_amount, char* in_signature) {

    //Transactions full
    if(in_chain->trans_index > 19)
        return;

    int index = in_chain->trans_index++;
    strcpy(in_chain->trans_list[index].sender, in_sender);
    strcpy(in_chain->trans_list[index].recipient, in_recipient);
    strcpy(in_chain->trans_list[index].signature, in_signature);
    in_chain->trans_list[index].amount = in_amount;

    //Update quick ledger
    //temp fix for creation transactions
    if(strcmp(in_sender, in_recipient)) {
        int* sender_funds = (int*)dict_access(in_chain->quickledger, in_sender);
        int sender_future_balance = *sender_funds - in_amount;
        dict_insert(in_chain->quickledger, in_sender, &sender_future_balance, sizeof(sender_funds));
    }

    void* recipient_funds = dict_access(in_chain->quickledger, in_recipient);
    int recipient_future_balance = 0;
    if(recipient_funds != NULL)
        recipient_future_balance = *((int*)recipient_funds);

    recipient_future_balance += in_amount;
    dict_insert(in_chain->quickledger, in_recipient, &recipient_future_balance, sizeof(recipient_future_balance));
    
     




}

//Create and blink_append new block
blink* append_current_block(blockchain* in_chain, long in_proof) {

    //Create block
    blink* the_block = blink_append(in_chain->head);

    //Add data
    the_block->data.index = ++(in_chain->length);
    the_block->data.time = time(NULL);
    memcpy(the_block->data.trans_list,in_chain->trans_list, sizeof(in_chain->trans_list));
    memcpy(the_block->data.posts, in_chain->new_posts, sizeof(the_block->data.posts));
    the_block->data.trans_list_length= in_chain->trans_index;
    the_block->data.proof = in_proof;
    memcpy(the_block->data.previous_hash,in_chain->last_hash, HASH_HEX_SIZE);

    //Register new block hash
    memcpy(in_chain->last_hash, hash_block(&the_block->data), HASH_HEX_SIZE);

    //Reset blockchain intake
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    in_chain->last_proof_of_work = in_proof;
    in_chain->trans_index = 0;

    if(in_chain->total_currency < CURRENCY_CAP)
        in_chain->total_currency += CURRENCY_SPEED;

    //Register as latest block
    char block_str[BLOCK_STR_SIZE];
    string_block(block_str, &the_block->data);
    strcpy(in_chain->last_block,block_str);

    return the_block;
}

blink* append_new_block(blockchain* in_chain, unsigned int index, unsigned int in_time, transaction* trans_list,
 char* posts, unsigned int trans_list_length, long proof) {

     //Create block
    blink* the_block = blink_append(in_chain->head);

    //Add data
    the_block->data.index = index;
    the_block->data.time = in_time;
    memcpy(the_block->data.trans_list,trans_list,sizeof(the_block->data.trans_list));
    memcpy(the_block->data.posts, in_chain->new_posts, sizeof(the_block->data.posts));
    the_block->data.trans_list_length= trans_list_length;
    the_block->data.proof = proof;
    memcpy(the_block->data.previous_hash,in_chain->last_hash, HASH_HEX_SIZE);

    //Register new block hash
    memcpy(in_chain->last_hash, hash_block(&the_block->data), HASH_HEX_SIZE);

    //Reset blockchain intake
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    in_chain->last_proof_of_work = proof;
    in_chain->trans_index = 0;
    ++(in_chain->length);

    //Register as latest block
    char block_str[BLOCK_STR_SIZE];
    string_block(block_str, &the_block->data);
    strcpy(in_chain->last_block,block_str);

    return the_block;



}

void print_block(blink* in_block, char separator)
{
    for(int i = 0; i < 77; i++)
        printf("%c", separator);
    printf("\n");
    
    printf("BLOCK # %d\n",in_block->data.index);
    printf("TIME: %d\n",in_block->data.time);
    printf("POSTS: %s\n",in_block->data.posts);
    printf("TRANSACTIONS:\n");

    for(int i = 0; i < in_block->data.trans_list_length; i++)
        printf("%.10s : %.10s - %d\n", in_block->data.trans_list[i].sender, in_block->data.trans_list[i].recipient, in_block->data.trans_list[i].amount);
    
    printf("PROOF: %ld\n",in_block->data.proof);
    printf("PREV HASH: ");
    printf("%s\n", in_block->data.previous_hash);
    /*
    for(int i = 0; i < 32; i++)
        printf("%02x",in_block->data.previous_hash[i]);
    printf("\n");
    */
    for(int i = 0; i < 77; i++)
        printf("%c", separator);
    printf("\n\n");
}

//Stringifies of the current block
char* string_block(char* output, block* in_block) {

    char block_string[BLOCK_STR_SIZE] = {0};
    char buffer[1100] = {0};

    //Add index and time
    sprintf(block_string,"%010i.%010i.", in_block->index, in_block->time);

    //Add posts
    if(strlen(in_block->posts) > 0) strcat(block_string, in_block->posts);

    //Add transactions
    for(int i = 0; i < in_block->trans_list_length; i++) {

        sprintf(buffer,"%s:%s:%010d", in_block->trans_list[i].sender,in_block->trans_list[i].recipient,in_block->trans_list[i].amount);
        if(i + 1 != in_block->trans_list_length) strcat(buffer,"-");
        strcat(block_string,buffer);
    }
    strcat(block_string,".");

    //Add transaction list length
    sprintf(buffer,"%02d.", in_block->trans_list_length);
    strcat(block_string, buffer);

    //Add proof
    sprintf(buffer,"%020ld.", in_block->proof);
    strcat(block_string, buffer);

    //Add previous hash
    strcat(block_string, in_block->previous_hash);

    /*
    for(int i = 0; i < HASH_SIZE; i++) {
        sprintf(buffer,"%02x", in_block->previous_hash[i]);
        strcat(block_string, buffer);
    }*/

    strcpy(output, block_string);

    return output;
}



int extract_transactions(blockchain* in_chain,transaction* trans_array, char* in_trans) {
 
    char* trans_strings[20] = {0};

    char* pointer = strtok(in_trans,"-");
    trans_strings[0] = pointer;
    int i = 1;
    while(pointer != NULL) {
        pointer = strtok(NULL,"-");
        trans_strings[i++] = pointer;
    }
    /*
    for(int i = 0; trans_strings[i] != 0; i++) {
        printf("TRANSACTION: %s\n", trans_strings[i]);
    }*/

    char* sender;
    char* reciever;
    char* amount;

    for(int i = 0; trans_strings[i] != 0; i++) {

        sender = strtok(trans_strings[i],":");
        //printf("sender: %s\n", sender);
        reciever = strtok(NULL, ":");
        //printf("reciever: %s\n", reciever);
        amount = strtok(NULL, ":");
        //printf("amount: %s\n", amount);
        strcpy(trans_array[i].sender, sender);
        strcpy(trans_array[i].recipient, reciever);
        trans_array[i].amount = atoi(amount);


        //Update quick ledger
        //temp fix for creation transactions
        if(strcmp(sender, reciever)) {
            int* sender_funds = (int*)dict_access(in_chain->quickledger, sender);
            int sender_future_balance = *sender_funds - atoi(amount);
            dict_insert(in_chain->quickledger, sender, &sender_future_balance, sizeof(sender_funds));
        }

        if(!strcmp(sender, reciever) && in_chain->total_currency > CURRENCY_CAP && atoi(amount) != 0) {
            return 1;
        }

        if(!strcmp(sender, reciever) && atoi(amount) != CURRENCY_SPEED) {
            return 1;
        }


        in_chain->total_currency += CURRENCY_SPEED;
        
        


        void* recipient_funds = dict_access(in_chain->quickledger, reciever);
        int recipient_future_balance = 0;
        if(recipient_funds != NULL)
            recipient_future_balance = *((int*)recipient_funds);

        recipient_future_balance += atoi(amount);
        dict_insert(in_chain->quickledger, reciever, &recipient_future_balance, sizeof(recipient_future_balance));




    }

    return 0;
}

char* hash_block(block* in_block) {

    char block_string[BLOCK_STR_SIZE];
    string_block(block_string, in_block);

    unsigned char* hash_value =  malloc(HASH_SIZE);
    hash256(hash_value, block_string);

    char* hash_hex = malloc(HASH_HEX_SIZE);
    memset(hash_hex, 0, HASH_HEX_SIZE);

    char buffer[3];
    for(int i = 0; i < HASH_SIZE; i++) {
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer,"%02x", hash_value[i]);
        strcat(hash_hex, buffer);
    }

    free(hash_value);


    return hash_hex;
}

bool valid_proof(char* last_hash, long proof) {

    char guess[GUESS_SIZE] = {0};
    sprintf(guess, "%s%020ld",last_hash, proof);

    unsigned char hash_value[HASH_SIZE];
    hash256(hash_value,guess);

    if(1)
        return (hash_value[0] == '0' && hash_value[1] == '0' /*&& hash_value[2] == '0' && (hash_value[3] > 60 && hash_value[3] < 127)*/);
    else
        return (hash_value[0] == '0' && hash_value[1] == '0' && hash_value[2] == '0' && (hash_value[3] > 60 && hash_value[3] < 127));

}

long proof_of_work(int* beaten, char* last_hash) {

    long proof = 0;

    while(valid_proof(last_hash, proof) == false){
        //printf("%d\n", proof);
        proof += 1;
        
        if(*beaten) {
            return -1;
        }
    }
    /*
    printf("OUR PROOF: %ld\n", proof);
    printf("CONFIRMATION: %d\n", valid_proof(last_hash, proof));

    char guess[GUESS_SIZE] = {0};
    sprintf(guess, "%s%020ld",last_hash, proof);*/

    return proof;
}

//Create keys for private_public pair
int create_keys(RSA** your_keys, char** pri_key, char** pub_key) {

    printf("PRI_KEY: %s\n", *pri_key);

//Create keypair
    *your_keys = RSA_generate_key(2048,3,NULL,NULL);

    //Create structures to seperate keys
    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    //Extract data out of RSA structure 
    PEM_write_bio_RSAPrivateKey(pri, *your_keys, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, *your_keys);

    //Get length of data
    size_t pri_len = BIO_pending(pri);
    size_t pub_len = BIO_pending(pub);

    //Prepare char buffers for keys
    *pri_key = malloc(pri_len + 1);
    *pub_key = malloc(pub_len + 1);

    //Read into buffers
    BIO_read(pri,*pri_key,pri_len);
    BIO_read(pub, *pub_key,pub_len);

    //Terminate strings
    (*pri_key)[pri_len] = '\0';
    (*pub_key)[pub_len] = '\0';


    return 1;

}

int destroy_keys(RSA** your_keys, char** pri_key, char** pub_key) {

    free(*pri_key);
    free(*pub_key);
    RSA_free(*your_keys);

    return 1;

}


int message_signature(char* output, char* message, RSA* keypair, char* pub_key) {


    //Hash the message
    unsigned char data[32];
    hash256(data,message + 2);

    //Print the hash
    printf("HASHVALUE:\n");
    for(int i= 0; i < 32; i++)
        printf("%02x", data[i]);
    printf("\n");

    //Prepare signature buffer
    unsigned char* sig = malloc(RSA_size(keypair));
    unsigned int sig_len = 0;
    if(sig == NULL) return 0;

    //Create signature
    int rc = RSA_sign(NID_sha256,data,32,sig, &sig_len,keypair);
    if(rc != 1) return 0;

    //convert to asci
    for(int i = 0; i < 256; i++) {
        char buf[3] = {0};
        sprintf(buf,"%02x", sig[i]);
        strcat(output,buf);
    }

    free(sig);
    printf("ASCI SIG:\n%s\n", output);

    //Verify
    unsigned char signature[256];
    char* pointer = output;
    //extract sig from hex asci
    for(int i = 0; i < 256; i++) {
        unsigned int value;
        sscanf(pointer, "%02x", &value);
        //printf("%02x", value);
        pointer = pointer + 2;
        signature[i] = value;
    }
    printf("\n");

    printf("size of key: %lu\n", strlen(pub_key) + 1);
    printf("%s\n", pub_key);

    BIO *bio = BIO_new_mem_buf((void*)pub_key, strlen(pub_key));
    RSA *rsa_pub = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);

    rc = RSA_verify(NID_sha256, data,32,signature,256,rsa_pub);
    if(rc != 1) printf("ERROR VERIFYING!\n"); else printf("VERIFIED!\n");



    return 1;
}

bool verify_signiture(const char* input, char* sender, char* recipient, char* amount, char* signature) {
    
    char data[1500] = {0};

    strcat(data, sender);
    strcat(data, " ");

    strcat(data, recipient);
    strcat(data," ");

    //char buffer[20] = {0};
    //sprintf(buffer, "%d", amount);
    strcat(data, amount);

    //printf("MESSAGE: %s\n", data);

    unsigned char hash_value[32];
    hash256(hash_value,data);
    /*printf("HASHVALUE:\n");
    for(int i= 0; i < 32; i++)
        printf("%02x", hash_value[i]);
    printf("\n");*/

    unsigned char sig[256];
    char* pointer = signature;
    //extract sig from hex asci
    for(int i = 0; i < 256; i++) {
        unsigned int value;
        sscanf(pointer, "%02x", &value);
        pointer = pointer + 2;
        sig[i] = value;
    }
    //printf("\n");

    //printf("\n\nASCI SIG:\n%s\n\n", signature);

    char our_key[1000] = {0};
    char* new_key_point = our_key;
    char* send_pointer = sender;

    for(int i = 0; i < 5; i++) {
        memcpy(new_key_point,send_pointer,64);
        new_key_point = new_key_point + 64;
        send_pointer = send_pointer + 64;
        *new_key_point++ = '\n';
    }

    memcpy(new_key_point,send_pointer,41);

    char final_key[427] = {0};
    sprintf(final_key,"-----BEGIN RSA PUBLIC KEY-----\n%s\n-----END RSA PUBLIC KEY-----\n", our_key);

    //printf("%s", final_key);

    char* pub_key = final_key;

    //printf("size of key: %lu\n", strlen(pub_key) + 1);
    //printf("\n%s\n", pub_key);

    BIO *bio = BIO_new_mem_buf((void*)pub_key, strlen(pub_key));
    RSA *rsa_pub = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);

    int rc = RSA_verify(NID_sha256, hash_value,32,sig,256,rsa_pub);
    //printf("VERIFY RETURN: %d\n", rc);
    if(rc != 1) printf("Invalid."); else printf("Valid.");

    if(rc) return true; else return false;
}


//Block-link FUNCTIONS:
blink* blink_create()
{
    blink* temp = malloc(sizeof(blink));
    temp->data.index = 0;
    temp->data.time = time(NULL);
    temp->data.proof = 0;
    memset(temp->data.trans_list,0,sizeof(temp->data.trans_list));
    memset(temp->data.previous_hash, 0, HASH_HEX_SIZE);
    temp->next = NULL;
    return temp;
}

blink* blink_prepend(blink* head)
{
    if(head == NULL)
        return head;

    blink* temp = blink_create();
    temp->next = head;
    head = temp;
    return head;
}

blink* blink_append(blink* head)
{
    blink* cursor = head;

    while(cursor->next != NULL)
        cursor = cursor->next;

    blink* temp = blink_create();
    cursor->next = temp;
    
    return temp;
}

blink* blink_remove_front(blink* head)
{
    blink* cursor = head;
    head = head->next;
    free(cursor);
    return head;
}

blink* blink_remove_end(blink* head)
{
    blink* cursor = head;
    blink* back = NULL;    
    
    while(cursor->next != NULL)
    {
        back = cursor;
        cursor = cursor->next;
    }

    if(back != NULL)
        back->next = NULL;

    free(cursor);
    
    return head;
}

void blink_print_list(blink* head)
{
    blink* temp = head;

    while(temp != NULL)
    {
        print_block(temp,'-');
        temp = temp->next;
    }
    printf("\n");

}

void blink_discard_list(blink* head)
{   
    if(head == NULL) return;
    //Single element sized list
    if(head->next == NULL)
    {
        free(head);
        return;
    }

    blink* temp = head;
    temp = temp->next;

    while(head != NULL)
    {
        free(head);
        head = temp;
        if(temp != NULL)
            temp = temp->next;
    }
}






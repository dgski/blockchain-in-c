#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <signal.h>
#include <openssl/sha.h>
#include <string.h>

#include "blockchain.h"

//Interface for 256 bit SHA hashing that takes a char* input
void hash256(unsigned char* output, const char* input) {

    size_t length = strlen(input);
    unsigned char md[32];
    SHA256((const unsigned char*)input, length, md);
    memcpy(output,md, 32);

    return;
}

//Create new Blockchain
blockchain* new_chain() {

    blockchain* in_chain = malloc(sizeof(blockchain));

    in_chain->head = blink_create();
    in_chain->head->data.time = 0;
    in_chain->head->data.proof = 100;
    in_chain->head->data.trans_list_length = 0;
    memset(in_chain->head->data.trans_list,0, sizeof(in_chain->head->data.trans_list));
    memset(in_chain->head->data.posts,0, sizeof(in_chain->head->data.posts));

    for(int i = 0; i < BLOCK_DATA_SIZE; i++) {
        in_chain->head->data.posts[i].data = '0';
    }
    in_chain->head->data.posts_list_length = 0;


    in_chain->last_proof_of_work = 100;
    memset(in_chain->trans_list, 0, sizeof(in_chain->trans_list));

    memset(in_chain->new_posts, 0, sizeof(in_chain->new_posts));

    for(int i = 0; i < BLOCK_DATA_SIZE; i++) {
        in_chain->new_posts[i].data = '0';
        in_chain->new_posts[i].poster[0] = '0';
        in_chain->new_posts[i].signature[0] = '0';
    }

    hash_block(in_chain->last_hash, &in_chain->head->data);
    
    in_chain->trans_index = 0;
    in_chain->post_index = 0;
    in_chain->length = 0;

    in_chain->quickledger = dict_create();
    in_chain->verified = dict_create();

    in_chain->total_currency = 0;
    
    char block[BLOCK_STR_SIZE];
    string_block(block,&(in_chain->head->data));
    strcpy(in_chain->last_block,block);



    return in_chain;
}

//Destroy the chain and free every link in it
int discard_chain(blockchain* in_chain) {

    if(in_chain == NULL) return 0;

    //Discard list of blocks in chain
    blink_discard_list(in_chain->head);

    //Discard quickledger
    dict_discard(in_chain->quickledger);

    //Discard verifications
    dict_discard(in_chain->verified);

    //Free memory in struct
    free(in_chain);

    return 1;
}

//Hash the transactions and posts provided
int hash_transactions(char* output, transaction* trans_array, unsigned int trans_array_length, post* post_array, unsigned int post_array_length) {

    if(output == NULL || trans_array == NULL || post_array == NULL) return 0;

    char all_transactions[BLOCK_STR_SIZE + 3000] ={0};

    for(int i = 0; i < trans_array_length; i++) {

        char this_transaction[BLOCK_BUFFER_SIZE];
        sprintf(this_transaction,"%d %s %s %d %s ", 
        trans_array[i].amount,
        trans_array[i].sender,
        trans_array[i].recipient,
        trans_array[i].amount,
        trans_array[i].signature
        );
        strcat(all_transactions,this_transaction);
    }

    for(int i = 0; i < post_array_length; i++) {

        char this_post[BLOCK_BUFFER_SIZE];
        sprintf(this_post,"%d %s %c %s",
        post_array[i].time_of,
        post_array[i].poster,
        post_array[i].data,
        post_array[i].signature
        );
        strcat(all_transactions,this_post);
    }

    unsigned char hashvalue[HASH_SIZE];
    hash256(hashvalue,all_transactions);

    char hex_hash[HASH_HEX_SIZE] = {0};
    char buffer[3];
    
    for(int i = 0; i < HASH_SIZE; i++) {
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer,"%02x", hashvalue[i]);
        strcat(hex_hash, buffer);
    }

    printf("HEXHASH OF TRANSACTIONS: '%s'\n", hex_hash);

    strcpy(output, hex_hash);


    return 1;

}

//Add a new a post to the given blockchain
void new_post(blockchain* in_chain, int time_of, char* in_sender, char in_data, char* in_signature) {

    //Posts full
    if(in_chain == NULL || in_sender == NULL || in_signature == NULL || in_chain->post_index > 7)
        return;


    int index = in_chain->post_index++;
    strcpy(in_chain->new_posts[index].poster, in_sender);
    in_chain->new_posts[index].data = in_data;
    in_chain->new_posts[index].time_of = time_of;
    strcpy(in_chain->new_posts[index].signature, in_signature);
    
    //Update Quick Ledger
    void* sender_funds = dict_access(in_chain->quickledger, in_sender);
    int sender_future_balance = 0;
    
    if(sender_funds == NULL) {
        printf("Cannot Insert Post: Quickledger entry does not exist.\n");
        return;
    }
    else {
        sender_future_balance = *((int*)sender_funds) - 1;
    } 
    
    sender_future_balance--;

    dict_insert(in_chain->quickledger, in_sender, &sender_future_balance, sizeof(sender_funds));

    hash_transactions(in_chain->trans_hash, in_chain->trans_list,in_chain->trans_index,in_chain->new_posts, in_chain->post_index);

    //Add to verified
    int status = 1;
    dict_insert(in_chain->verified, in_signature, &status,sizeof(status));

}

//Add transaction to transaction_list
void new_transaction(blockchain* in_chain, int time_of, char* in_sender, char* in_recipient, int in_amount, char* in_signature) {

    //Transactions full
    if(in_chain == NULL || in_sender == NULL || in_recipient == NULL || in_signature == NULL || in_chain->trans_index > 19)
        return;

    int index = in_chain->trans_index++;
    strcpy(in_chain->trans_list[index].sender, in_sender);
    strcpy(in_chain->trans_list[index].recipient, in_recipient);

    strcpy(in_chain->trans_list[index].signature, in_signature);

    in_chain->trans_list[index].amount = in_amount;
    in_chain->trans_list[index].time_of = time_of;

    //Update quick ledger
    if(strcmp(in_sender, in_recipient)) {
        void* sender_funds = dict_access(in_chain->quickledger, in_sender);
        int sender_future_balance = 0;

        if(sender_funds != NULL) {
            sender_future_balance = *((int*)sender_funds) - in_amount; //CHANGE AF
        }
        else {
            return;
        }
        dict_insert(in_chain->quickledger, in_sender, &sender_future_balance, sizeof(sender_funds));
    }

    void* recipient_funds = dict_access(in_chain->quickledger, in_recipient);
    int recipient_future_balance = 0;
    if(recipient_funds != NULL)
        recipient_future_balance = *((int*)recipient_funds);

    recipient_future_balance += in_amount;
    dict_insert(in_chain->quickledger, in_recipient, &recipient_future_balance, sizeof(recipient_future_balance));
    
    hash_transactions(in_chain->trans_hash, in_chain->trans_list,in_chain->trans_index,in_chain->new_posts, in_chain->post_index);

    //Add to verified
    int status = 1;
    dict_insert(in_chain->verified, in_signature, &status,sizeof(status));

}

//Create and blink_append new block to the given chain from ready parameters in chain
blink* append_current_block(blockchain* in_chain, long in_proof) {

    if(in_chain == NULL) return NULL;

    //Create block
    blink* the_block = blink_append(in_chain->head);

    //Add data
    the_block->data.index = ++(in_chain->length);
    the_block->data.time = time(NULL);
    memcpy(the_block->data.trans_list,in_chain->trans_list, sizeof(in_chain->trans_list));
    memcpy(the_block->data.posts, in_chain->new_posts, sizeof(the_block->data.posts));
    the_block->data.trans_list_length = in_chain->trans_index;
    the_block->data.posts_list_length = in_chain->post_index;
    the_block->data.proof = in_proof;
    memcpy(the_block->data.previous_hash,in_chain->last_hash, HASH_HEX_SIZE);

    //Register new block hash
    hash_block(in_chain->last_hash, &in_chain->head->data);


    //Reset blockchain intake
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    memset(in_chain->new_posts,0, sizeof(in_chain->new_posts));
    in_chain->last_proof_of_work = in_proof;
    in_chain->trans_index = 0;
    in_chain->post_index = 0;

    if(in_chain->total_currency < CURRENCY_CAP)
        in_chain->total_currency += CURRENCY_SPEED;

    //Register as latest block
    char block_str[BLOCK_STR_SIZE];
    string_block(block_str, &the_block->data);
    strcpy(in_chain->last_block,block_str);

    return the_block;
}

//Create and blink_append new block ot the given chain from the parameters given
blink* append_new_block(blockchain* in_chain, unsigned int index, unsigned int in_time, transaction* trans_list,
 post* posts, unsigned int trans_list_length, unsigned int posts_list_length, long proof) {

     //Create block
    blink* the_block = blink_append(in_chain->head);

    //Add data
    the_block->data.index = index;
    the_block->data.time = in_time;
    memcpy(the_block->data.trans_list,trans_list,sizeof(the_block->data.trans_list));
    memcpy(the_block->data.posts, posts, sizeof(the_block->data.posts));
    the_block->data.trans_list_length= trans_list_length;
    the_block->data.posts_list_length = posts_list_length;
    the_block->data.proof = proof;
    memcpy(the_block->data.previous_hash,in_chain->last_hash, HASH_HEX_SIZE);

    //Register new block hash
    hash_block(in_chain->last_hash, &in_chain->head->data);


    //Reset blockchain intake
    memset(in_chain->trans_list,0, sizeof(in_chain->trans_list));
    memset(in_chain->new_posts, 0,sizeof(in_chain->new_posts));
    in_chain->last_proof_of_work = proof;
    in_chain->trans_index = 0;
    in_chain->post_index = 0;
    ++(in_chain->length);

    //Register as latest block
    char block_str[BLOCK_STR_SIZE];
    string_block(block_str, &the_block->data);
    strcpy(in_chain->last_block,block_str);

    return the_block;



}

//Print out the given block using the following seperator for decoration
void print_block(blink* in_block, char separator) {
    if(in_block == NULL) return;

    for(int i = 0; i < 77; i++)
        printf("%c", separator);
    printf("\n");
    
    printf("BLOCK # %d\n",in_block->data.index);
    time_t raw_time = in_block->data.time;
    printf("TIME: %s",ctime(&raw_time));
    printf("POSTS: ");
    for(int i = 0; i < in_block->data.posts_list_length; i++)
        printf("%c",in_block->data.posts[i].data);
    printf("\n");
    printf("TRANSACTIONS:\n");

    for(int i = 0; i < in_block->data.trans_list_length; i++)
        printf("%.10s... : %.10s... - %d , %.10s...\n", in_block->data.trans_list[i].sender + 12, in_block->data.trans_list[i].recipient + 12, 
        in_block->data.trans_list[i].amount, in_block->data.trans_list[i].signature);
    
    printf("PROOF: %ld\n",in_block->data.proof);
    printf("PREV HASH: ");
    printf("%s\n", in_block->data.previous_hash);
    for(int i = 0; i < 77; i++)
        printf("%c", separator);
    printf("\n\n");
}

//Stringifies of the current block
char* string_block(char* output, block* in_block) {

    if(in_block == NULL || output == NULL) return NULL;

    char block_string[BLOCK_STR_SIZE] = {0};
    char buffer[BLOCK_BUFFER_SIZE] = {0};

    //Add index and time
    sprintf(block_string,"%010i.%010i.", in_block->index, in_block->time);

    if(in_block->posts_list_length == 0) {
        strcat(block_string, "0");
    }
    else {
    //Add posts
    for(int i = 0; i < in_block->posts_list_length; i++) {
        sprintf(buffer,"%d:%s:%c:%s",in_block->posts[i].time_of,in_block->posts[i].poster, in_block->posts[i].data,in_block->posts[i].signature); 
        if(i + 1 != in_block->posts_list_length) strcat(buffer,"-");
        strcat(block_string, buffer);
    }
    }
    strcat(block_string, ".");

    //Add posts list length
    sprintf(buffer,"%01d.", in_block->posts_list_length);
    strcat(block_string, buffer);

    //Add transactions
    for(int i = 0; i < in_block->trans_list_length; i++) {

        memset(buffer, 0, BLOCK_BUFFER_SIZE);
        sprintf(buffer,"%d:%s:%s:%010d:%s", in_block->trans_list[i].time_of, in_block->trans_list[i].sender,in_block->trans_list[i].recipient,in_block->trans_list[i].amount, in_block->trans_list[i].signature);
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
    strcpy(output, block_string);

    return output;
}

//Stringify the following transaction (without signature)
char* string_trans_nosig(char* output, int time_of, char* sender, char* reciever, int amount) {

    if(output == NULL || sender == NULL || reciever == NULL) return NULL;

    sprintf(output,"%d %s %s %010d ",time_of, sender, reciever, amount);
    return output;
}

//Stringify the following post (without signature)
char* string_post_nosig(char* output, int time_of, char* sender, char data) {

    if(output == NULL || sender == NULL) return NULL;

    sprintf(output,"%d %s %c ",time_of,sender, data);

    return output;
}

//Extract the posts from the given string and place them into post array
int extract_posts_raw(post* post_array, char* input_posts_string) {

    if(post_array == NULL || input_posts_string == NULL) return 0;
    if(input_posts_string[0] == '0') return 0;

    char* post_strings[BLOCK_DATA_SIZE] = {0};
    char in_posts[BLOCK_STR_SIZE] = {0};
    strcpy(in_posts, input_posts_string);
    char* pointer = strtok(in_posts,"-");
    post_strings[0] = pointer;
    int i = 1;
    int counter = 1;
    while(pointer != NULL) {
        pointer = strtok(NULL,"-");
        post_strings[i++] = pointer;
        counter++;
    }

    char* time_of;
    char* poster;
    char* data;
    char* signature;

    for(int i = 0; post_strings[i] != 0; i++) {

        time_of = strtok(post_strings[i],":");
        poster = strtok(NULL,":");
        data = strtok(NULL, ":");
        signature = strtok(NULL, ":");

        post_array[i].time_of = atoi(time_of);
        strcpy(post_array[i].poster, poster);
        post_array[i].data = *data;
        strcpy(post_array[i].signature, signature);

    }

    return counter;
}


//Extract the transactions from the given string and place them into trans array
int extract_transactions_raw(transaction* trans_array, char* input_trans_string) {


    char* trans_strings[20] = {0};

    char in_trans[30000] = {0};
    strcpy(in_trans, input_trans_string);

    char* pointer = strtok(in_trans,"-");
    trans_strings[0] = pointer;
    int i = 1;
    while(pointer != NULL) {
        pointer = strtok(NULL,"-");

        if(pointer == NULL) break;

        trans_strings[i++] = pointer;
    }

    char* time_of;
    char* sender;
    char* reciever;
    char* amount;
    char* signature;

    for(int i = 0; trans_strings[i] != 0; i++) {

        printf("TRANSACTION #%d:\n", i);
        time_of = strtok(trans_strings[i],":");
        sender = strtok(NULL, ":");
        reciever = strtok(NULL, ":");
        amount = strtok(NULL, ":");
        signature = strtok(NULL, ":");


        if(time_of == NULL || sender == NULL || reciever == NULL || amount == NULL || signature == NULL) {
            printf("RETURNING ZERO!\n");
            return 0;

        }
        /*
        printf("time_of: %s", time_of);
        printf("sender: %s\n", sender);
        printf("reciever: %s\n", reciever);
        printf("amount: %s\n", amount);
        printf("signature: %s\n", signature);
        */

        char output[10000] = {0};
        string_trans_nosig(output,atoi(time_of),sender,reciever,atoi(amount));

        trans_array[i].time_of = atoi(time_of);
        strcpy(trans_array[i].sender, sender);
        strcpy(trans_array[i].recipient, reciever);
        trans_array[i].amount = atoi(amount);
        strcpy(trans_array[i].signature, signature);

    }

    return 1;
}

//Validate the posts in the given array
int validate_posts(blockchain* in_chain, post* new_post_array, int nr_of_posts) {

    if(nr_of_posts == 0 || in_chain == NULL || new_post_array == NULL) return 1;

    for(int i = 0; i < nr_of_posts; i ++) {
        char output[1500] = {0};
        string_post_nosig(output, new_post_array[i].time_of, new_post_array[i].poster, new_post_array[i].data);

        if(!verify_message(output,new_post_array[i].poster, new_post_array[i].signature))
            return 0;

        //Add to verified
        int status = 1;
        dict_insert(in_chain->verified, new_post_array[i].signature, &status,sizeof(status));
    }

    return 1;
    
}

//Extract the transactions from the given string, place them into the given array and then add them to the given blockchain 
int extract_transactions(blockchain* in_chain,transaction* trans_array, const char* in_trans) {

    char the_trans_chars[30000];
    strcpy(the_trans_chars,in_trans);


    char* trans_strings[20] = {0};

    char* pointer = strtok(the_trans_chars,"-");
    trans_strings[0] = pointer;
    int i = 1;
    while(pointer != NULL) {
        pointer = strtok(NULL,"-");
        trans_strings[i++] = pointer;
    }

    char* time_of;
    char* sender;
    char* reciever;
    char* amount;
    char* signature;

    for(int i = 0; trans_strings[i] != 0; i++) {

        printf("TRANSACTION #%d:\n", i);
        time_of = strtok(trans_strings[i],":");
        sender = strtok(NULL, ":");
        reciever = strtok(NULL, ":");
        amount = strtok(NULL, ":");
        signature = strtok(NULL, ":");

        if(time_of == NULL || sender == NULL || reciever == NULL || amount == NULL || signature == NULL) {
            printf("RETURNING ZERO!\n");
            return 0;
        }

        printf("time_of: %s", time_of);
        printf("sender: %s\n", sender);
        printf("reciever: %s\n", reciever);
        printf("amount: %s\n", amount);
        printf("signature: %s\n", signature);

        char output[10000] = {0};
        string_trans_nosig(output, atoi(time_of),sender,reciever,atoi(amount));

        printf("VERIFYING TRANSACTION:\n");
        if(!verify_message(output,sender,signature))
            return 0;


        trans_array[i].time_of = atoi(time_of);
        strcpy(trans_array[i].sender, sender);
        strcpy(trans_array[i].recipient, reciever);
        trans_array[i].amount = atoi(amount);
        strcpy(trans_array[i].signature, signature);


        //Update quick ledger
        //temp fix for creation transactions
        if(strcmp(sender, reciever)) {
            void* sender_funds = dict_access(in_chain->quickledger, sender);
            int sender_future_balance = 0;
            if(sender_funds != NULL)
                sender_future_balance = *((int*)sender_funds) - atoi(amount);

            dict_insert(in_chain->quickledger, sender, &sender_future_balance, sizeof(sender_funds));
        }

        //Addresses are the same, Currency cap already met, and they are trying to give themselves more
        if(!strcmp(sender, reciever) && in_chain->total_currency == CURRENCY_CAP && atoi(amount) != 0) {
            return 0;
        }

        //Addresses are the same, Trying to givethemselves more than 2
        if(!strcmp(sender, reciever) && (in_chain->total_currency < CURRENCY_CAP) && (atoi(amount) != CURRENCY_SPEED) ) {
            return 0;
        }

        //Behold! The creation of NOINCOINS!
        if(in_chain->total_currency < CURRENCY_CAP)
            in_chain->total_currency += CURRENCY_SPEED;
        

        void* recipient_funds = dict_access(in_chain->quickledger, reciever);
        int recipient_future_balance = 0;
        if(recipient_funds != NULL)
            recipient_future_balance = *((int*)recipient_funds);

        recipient_future_balance += atoi(amount);
        dict_insert(in_chain->quickledger, reciever, &recipient_future_balance, sizeof(recipient_future_balance));

        //Add to verified
        int status = 1;
        dict_insert(in_chain->verified, signature, &status,sizeof(status));


    }

    return 1;
}

//Hash the given block
char* hash_block(char* output, block* in_block) {

    char block_string[BLOCK_STR_SIZE];
    string_block(block_string, in_block);

    unsigned char hash_value[HASH_SIZE];
    hash256(hash_value, block_string);

    char buffer[3];
    char hex_hash[HASH_HEX_SIZE] = {0};
    for(int i = 0; i < HASH_SIZE; i++) {
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer,"%02x", hash_value[i]);
        strcat(hex_hash, buffer);
    }

    strcpy(output,hex_hash);

    output[HASH_HEX_SIZE] = 0;

    return output;
}

//Check whether the given proof is valid
bool valid_proof(char* last_hash, char* trans_hash,  long proof) {

    char guess[GUESS_SIZE] = {0};
    sprintf(guess, "%s%s%020ld",last_hash, trans_hash, proof);
    //printf("%s\n", guess);

    unsigned char hash_value[HASH_SIZE];
    hash256(hash_value,guess);

    if(0) //Debug mode
        return (hash_value[0] == '0' && hash_value[1] == '0' /* && hash_value[2] == '0'  && (hash_value[3] > 60 && hash_value[3] < 127)*/);
    else
        return (hash_value[0] == '0' && hash_value[1] == '0' && hash_value[2] == '0' && (hash_value[3] > 60 && hash_value[3] < 127));

}

//Proof of work function that searches for right proof. Exits if the node is beaten, or exit signal is sent
long proof_of_work(int* beaten, char* last_hash, char* trans_hash) {

    long proof = 0;
    char old_trans_hash[HASH_HEX_SIZE];
    strcpy(old_trans_hash, trans_hash);

    while(valid_proof(last_hash, old_trans_hash, proof) == false){
        //printf("%d\n", proof);
        proof += 1;
        
        if(*beaten == 1) {
            return -1;
        }
        if(*beaten == 2) {
            printf("Shutting Down Mining.\n");
            return -2;
        }
        if(strcmp(old_trans_hash, trans_hash)) {
            printf("Hash Changed. Resetting Nonce.\n");
            proof = 0;
            strcpy(old_trans_hash, trans_hash);
        }
    }
    return proof;
}

//Read keys from the provided file
int read_keys(RSA** our_keys, char* pri_filename, char* pub_filename) {

    *our_keys = RSA_new();

    //Read private key
    FILE* pri_key_file = fopen(pri_filename, "r");
    if(pri_key_file == NULL) return 0;

    RSA* pointer = PEM_read_RSAPrivateKey(pri_key_file, our_keys, NULL, NULL);
    fclose(pri_key_file);

    printf("\n");
    
    //Read public key
    FILE* pub_key_file = fopen(pri_filename, "r");
    if(pub_key_file == NULL) return 0;

    PEM_read_RSAPublicKey(pub_key_file, our_keys, NULL, NULL);

    fclose(pub_key_file);

    return 1;
}

//Write keys to the provided file
int write_keys(RSA** our_keys, char* pri_filename, char* pub_filename) {
    

    //Write private key
    FILE* pri_key_file = fopen(pri_filename, "w");
    if(pri_key_file == NULL) return 0;
    PEM_write_RSAPrivateKey(pri_key_file, *our_keys, NULL, NULL, 0, NULL, NULL);
    fclose(pri_key_file);
    
    
    //Write public key
    FILE* pub_key_file = fopen(pub_filename, "w");
    if(pub_key_file == NULL) return 0;
    PEM_write_RSAPublicKey(pub_key_file, *our_keys);
    fclose(pub_key_file);


    return 1;

}

//Create keys for private_public pair
int create_keys(RSA** your_keys, char** pri_key, char** pub_key) {

    //Create keypair if not provided
    if(*your_keys == NULL)
        *your_keys = RSA_generate_key(2048,65535,NULL,NULL);

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

    //Free memory
    BIO_vfree(pri);
    BIO_vfree(pub);


    return 1;

}

//Destroys the provided keys
int destroy_keys(RSA** your_keys, char** pri_key, char** pub_key) {

    free(*pri_key);
    free(*pub_key);

    if(*your_keys ==  NULL) return 1;
    
    RSA_free(*your_keys);

    return 1;

}

//Creates a signature with the given sender and message
int message_signature(char* output, char* message, RSA* keypair, char* pub_key) {


    //Hash the message
    unsigned char data[32];
    hash256(data,message);

    //Prepare signature buffer
    unsigned char sig[RSA_size(keypair)];
    unsigned int sig_len = 0;

    //Create signature
    int rc = RSA_sign(NID_sha256,data,32,sig, &sig_len,keypair);
    if(rc != 1) return 0;

    //convert to asci
    for(int i = 0; i < 256; i++) {
        char buf[3] = {0};
        sprintf(buf,"%02x", sig[i]);
        strcat(output,buf);
    }

    char stripped_key[1000];
    strip_pub_key(stripped_key,pub_key);
    rc = verify_message(message, stripped_key, output);
    if(rc != 1) printf("ERROR VERIFYING!\n"); else printf("VERIFIED!\n");



    return 1;
}

//Verifies whether the message is signed by the sender given the following signature
bool verify_message(const char* input, char* sender, char* signature) {

    unsigned char hash_value[32];
    hash256(hash_value,input);

    unsigned char sig[256];
    char* pointer = signature;
    //extract sig from hex asci
    for(int i = 0; i < 256; i++) {
        unsigned int value;
        sscanf(pointer, "%02x", &value);
        pointer = pointer + 2;
        sig[i] = value;
    }

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

    char* pub_key = final_key;

    BIO *bio = BIO_new_mem_buf((void*)pub_key, strlen(pub_key));
    RSA *rsa_pub = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);

    int rc = RSA_verify(NID_sha256, hash_value,32,sig,256,rsa_pub);

    BIO_free_all(bio);
    RSA_free(rsa_pub);


    //printf("VERIFY RETURN: %d\n", rc);
    if(rc != 1) printf("Invalid.\n"); else printf("Valid.\n");

    if(rc) return true; else return false;

    return false;
}

//Strips the RSA public key of the newlines, header and footer
int strip_pub_key(char* output, char* pub_key) {
    char asci_pub_key[500] = {0};
    int i = 31;
    int x = 0;
    while (pub_key[i] != '-') {
        if(pub_key[i] != '\n') asci_pub_key[x++] = pub_key[i];
        i++;
    }

    //printf("length: %lu\n", strlen(asci_pub_key));
    //printf("PUBLIC KEY STRIPPED: %s\n", asci_pub_key);

    strcpy(output, asci_pub_key);

    return 0;
}

//////////////////////////////////
//Block-link FUNCTIONS:
/////////////////////////////////
blink* blink_create()
{
    blink* temp = malloc(sizeof(blink));
    temp->data.index = 0;
    temp->data.time = time(NULL);
    temp->data.proof = 0;
    temp->data.posts_list_length = 0;
    temp->data.trans_list_length = 0;
    memset(temp->data.trans_list,0,sizeof(temp->data.trans_list));
    memset(temp->data.posts,0, sizeof(temp->data.posts));
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

    head = NULL;
}






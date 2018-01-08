#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "hash.h"

int main(void) {

    char hexhash[] = "bf14d98cf25e31b5f7ae060ddad10d0b5fbab401db7173044a066ddd99f9ea24";
    char* point = hexhash;
    unsigned char last_hash[32] = {0};

    int p = 0;
    for(int i = 0; i < 32; i++) {

        unsigned int value;
        sscanf(point,"%02x",&value);
        point = point + 2;
        printf("%u, ", value);
        last_hash[p++] =  value;
    }
    printf("\n");

    for(int i = 0; i < 32; i++)
        printf("%02x",last_hash[i]);

    printf("\n");

    long proof = 16876997;

    char guess[120] = {0};
    sprintf(guess, "%s%020lu",last_hash, proof);

    unsigned char tester[32] = {0};
    hash256(tester,guess);
    printf("generated hash: ");
    for(int i = 0; i < 32; i++)
        printf("%02x",tester[i]);
    printf("\n");


}
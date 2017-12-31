#include <openssl/sha.h>
#include <string.h>

void hash256(unsigned char* output, const char* input) {

    size_t length = strlen(input);
    unsigned char md[32];
    SHA256((const unsigned char*)input, length, md);

    memcpy(output,md, 32);

    //printf("sizeof result: %lu\n", sizeof(result));

    /*
    for(int i = 0; i < 32; i++) {
        printf("%d.",result[i]);
    }
    printf("\n");
    */

    return;
    //return result;
}
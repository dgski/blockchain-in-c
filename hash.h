#include <openssl/sha.h>
#include <string.h>

void hash256(unsigned char* output, const char* input) {

    size_t length = strlen(input);
    unsigned char md[32];
    SHA256((const unsigned char*)input, length, md);
    memcpy(output,md, 32);

    return;
}

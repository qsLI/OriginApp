#define HW 5

typedef struct {
    unsigned long state[5];
    unsigned long count[2];
    unsigned char buffer[64];
} hash_context;

void hash_initial( hash_context * c );
void hash_process( hash_context * c, unsigned char * data, unsigned len );
void hash_final( hash_context * c, unsigned long digest[HW] );
void sha1_hash_full(unsigned long digest[5], unsigned char * data, unsigned len);

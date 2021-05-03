
#define HASH_SIZE (1<<12)
#define W 48

#define L1_SIZE (1UL<<15) //32KB per core
#define L2_SIZE (1UL<<19) //512KB per core
#define LLC_SIZE (1UL<<21) //2MB per core

unsigned int start_hi, start_lo, end_hi, end_lo;

// Use the preprocessor so we know definitively that these are placed inline
#define RDTSC_START()\
    __asm__ volatile("RDTSCP\n\t" \
    "mov %%edx, %0\n\t" \
    "mov %%eax, %1\n\t" \
    : "=r" (start_hi), "=r" (start_lo) \
    :: "%rax", "%rbx", "%rcx", "%rdx");

#define RDTSC_STOP()\
    __asm__ volatile("RDTSCP\n\t" \
    "mov %%edx, %0\n\t" \
    "mov %%eax, %1\n\t" \
    "CPUID\n\t" \
    : "=r" (end_hi), "=r" (end_lo) \
    :: "%rax", "%rbx", "%rcx", "%rdx");

// Returns the elapsed time given the high and low bits of the start and stop time.

unsigned long start, end, time_taken;

#define ELAPSED_TIME(start_high,start_low,end_high,end_low)\
    do{ \
        start = ((((uint64_t)start_high) << 32) | start_low);\
        end   = ((((uint64_t)end_high)   << 32) | end_low);\
        time_taken = (end-start);}\
    while(0)


struct hash_entry{
    unsigned int id;
    struct element * id_loc;
    struct hash_entry * next;
};


struct element{
    char * value;
    unsigned int size;
    struct element * next;
};
struct hash_entry * hash_table_elements;
struct element * list_elements;
char * elem_value_chunk;
struct element * head;
struct element * tail;
struct hash_entry * hash_table;
void insert_hashtable(unsigned int key, char * value, unsigned int size);
void insert_list(struct element * node, char * value, unsigned int size);



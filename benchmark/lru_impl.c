/**
  **************
  This program implements LRU list with Hashtable based look-up with transactional capability.

  |-------|
  |  ##   |-----------|
  |-------|           |
  |       |           |
  |-------|           |
                     \|/
       HEAD  |----|-----|----|-----| TAIL
             |    |  ## |    |     |
             |----|-----|----|-----|

  REDO logging is used to provide memory conistency.

This program takes input as
***************************
Type: defines the total size of datat structure, values are l1f, l2f, llcf, llcnf, applicable only for Size argument <=64
Number of operations: Number of total read & write operations inside a transaction. eg 1000
Size: size of the value in bytes stored in LRU list, eg: 64, 128, 512
Write percentage: Percentage of write operations inside the transaction. eg 20%
Consistency Mechanism: flush mechanism used to ensure consistency
0: no flushing
1: clflush
2: clflushopt
3: clwb

Example run:

./lru llcf 1000 64 20 0
./lru llcnf 1000 512 50 1

************************
Author: KP Arun
Institution: IIT Kanpur
Date: 16/04/2021
Time: Covid Days
***********************
**/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <math.h>
#include "lru_impl.h"
#include "murmur3.h"
#include "redo.h"

void gen_seed(unsigned int * seed){
    unsigned long num;
    do{
        num = rand()%((1UL<<W)-1);
        }while(num%2==0);
        *seed = num;
}

void insert_list(struct element * node, char * value,unsigned int size){
    if(head == NULL){
        node->value = value;
        node->size = size;
        node->next = NULL;
        head = node;
        tail = node;
    }
    else{
        node->value = value;
        node->size = size;
        node->next = head;
        head = node;
    }
}

struct hash_entry * hash_elem_alloc(){
    static unsigned int hindex = 0;
    struct hash_entry * temp = &(hash_table_elements[hindex]);
    hindex += 1;
    return temp;
}

struct element * list_elem_alloc(){
    static unsigned int lindex = 0;
    struct element * temp = &(list_elements[lindex]);
    lindex += 1;
    return temp;
}
char * chunk_alloc(unsigned int size){
    static unsigned int cindex = 0;
    char * temp = &(elem_value_chunk[cindex*size]);
    cindex += 1;
    return temp;
}

struct log_entry * log_entry_alloc(){
    static unsigned int lindex = 0;
    struct log_entry * temp = &(log_entries[lindex]);
    lindex += 1;
    return temp;
}

void insert_hashtable(unsigned int key, char * value, unsigned int size){
    unsigned int hash_index = key%HASH_SIZE;
    if(hash_table[hash_index].id == 0){
        hash_table[hash_index].id = key;
        hash_table[hash_index].id_loc = (struct element *)list_elem_alloc();
        insert_list(hash_table[hash_index].id_loc,value,size);
        hash_table[hash_index].next = NULL;
    }
    else{
        struct hash_entry * temp = &(hash_table[hash_index]);
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = (struct hash_entry*)hash_elem_alloc();
        (temp->next)->id = key;
        (temp->next)->id_loc = (struct element *)list_elem_alloc();
        insert_list((temp->next)->id_loc,value,size);
        (temp->next)->next = NULL;
    }
}

/**
  *****************
  access method moves the element with id to head of LRU list.
  It also updates the value of element.
  Makes an REDO Log entry of new value to ensure consistency.
  REDO LOG is applied at end transaction. 
  *****************
  **/

void access_id(unsigned int id){
   unsigned int hash_index = id%HASH_SIZE;
   struct hash_entry * temp = &hash_table[hash_index];
   while(temp->id != id){
       temp = temp->next;
   }
   struct element * elem_loc = temp->id_loc;
   if( head == elem_loc){
       return;
   }
   struct element * temp_elem = head;
   while(temp_elem->next != elem_loc){
       temp_elem = temp_elem->next;
   }

   //temp_elem->next = elem_loc->next;
   INSERT_LOG(&(temp_elem->next),&(elem_loc->next),8);
   //elem_loc->next = head;
   INSERT_LOG(&(elem_loc->next),&head,8);
   //memset(elem_loc->value,'a',elem_loc->size);
   char * temp_value = chunk_alloc(elem_loc->size);
   memset(temp_value, 'a', elem_loc->size);
   INSERT_LOG(elem_loc->value,temp_value,elem_loc->size);
   //head = elem_loc;
   INSERT_LOG(&head,&elem_loc,8);
   return;
}
/***
  ************
  peek method returns the position of id in the LRU list.
  This is a simple read operation so no memory consistency is required
  ***********
  **/

unsigned int peek(unsigned int id){ 
   unsigned int hash_index = id%HASH_SIZE;
   struct hash_entry * temp = &hash_table[hash_index];
   unsigned int current_loc = 0;
   while(temp->id != id){
       temp = temp->next;
   }
   struct element * elem_loc = temp->id_loc;
   struct element * temp_elem = head;
   while(temp_elem != elem_loc){
       current_loc +=1;
       temp_elem = temp_elem->next;
   }
   return current_loc;   
}

int main(int argc, char* argv[]){

    if( (argv[1] && strcmp(argv[1],"--help") == 0) || (argv[1] && strcmp(argv[1],"--h") == 0) || argc<6){
        printf("pass type, number of ops, size(bytes) of value, write_per, consistency mechanism\n");
        printf("0: no flush\n");
        printf("1: clfush\n");
        printf("2: clfushopt\n");
        printf("3: clwb\n");
        exit(0);}
  
    unsigned int operations = atoi(argv[2]);
    unsigned int size = atoi(argv[3]);
    float write_per = atof(argv[4])/100.0;
    con_mech = atoi(argv[5]);
    unsigned int elements;
    unsigned total_size = (sizeof(struct hash_entry)+sizeof(struct element)+(size*sizeof(char)));
    
    if(size<=64){
    if(!strcmp(argv[1],"l1f"))
        elements = 0.9*floor(L1_SIZE/total_size);
    else if(!strcmp(argv[1],"l2f"))
        elements = 0.9*floor(L2_SIZE/total_size);
    else if(!strcmp(argv[1],"llcf"))
        elements = 0.9*floor(LLC_SIZE/total_size);
    else if(!strcmp(argv[1],"llcnf"))
        elements = 4*floor(LLC_SIZE/total_size);
    else{
         printf("pass a valid data type\n");
         printf("l1f: L1 FITTING\n");
         printf("l2f: L2 FITTING\n");
         printf("llcf: LLC FITTING\n");
         printf("llcnf: LLC NOT FITTING\n");
         exit(0);
        }}
    else{ elements = 67408;}


    char * master_bitmap = (char *)mmap(NULL,5*elements*sizeof(char),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(master_bitmap == MAP_FAILED){
        perror("mmap failed");
        exit(-1);
    }

    int * insert_elements = (int *)mmap(NULL,elements*sizeof(unsigned int),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(insert_elements == MAP_FAILED){
        perror("mmap failed");
        exit(-1);
    }

    srand(0);
    int q = 0,r = 0;
    unsigned int loop_index, rand_index;
    for(loop_index = 0; loop_index < elements; loop_index++){
        do{
            rand_index = rand()%(5*elements*sizeof(char));
            q = rand_index>>3;
            r = rand_index%8;
           }while(*(master_bitmap+q) & 1<<(7-r));
        insert_elements[loop_index] = rand_index+1;
        *(master_bitmap+q) |= 1<<(7-r);
    }
    
    hash_table = (struct hash_entry *)mmap(NULL,HASH_SIZE*sizeof(struct hash_entry),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE,-1,0);
    if(hash_table == MAP_FAILED){
        perror("mmap failed");
        exit(-1);
    }

    for(loop_index = 0; loop_index<HASH_SIZE; loop_index++){
        hash_table[loop_index].id = 0;
        hash_table[loop_index].id_loc = NULL;
        hash_table[loop_index].next = NULL;
    }
    
    hash_table_elements = (struct hash_entry *)mmap(NULL,(elements-HASH_SIZE)*sizeof(struct hash_entry),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE,-1,0);
    if(hash_table_elements == MAP_FAILED){
        perror("mmap failed");
        exit(-1);
    }

    list_elements = (struct element *)mmap(NULL,elements*sizeof(struct element),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE,-1,0);
    if(list_elements == MAP_FAILED){
        perror("mmap failed");
        exit(-1);
    }

    elem_value_chunk = (char*)mmap(NULL,3*elements*size*sizeof(char),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE,-1,0);
    if(elem_value_chunk == MAP_FAILED){
        perror("mmap failed");
        exit(-1);
    }

    log_entries = (struct log_entry*)mmap(NULL,elements*sizeof(struct log_entry),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE,-1,0);
    if(log_entries == MAP_FAILED){
        perror("mmap failed");
        exit(-1);
    }

    char * operation_bitmap = (char *)mmap(NULL,operations*sizeof(char),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    
    if(operation_bitmap == MAP_FAILED){
        perror("mmap failed");
        exit(-1);
    }

    memset(operation_bitmap,0,operations*sizeof(char));
    unsigned int writes = operations*write_per;
    srand(11);
    for(loop_index = 0; loop_index < writes; ){
        unsigned int turn = rand()%operations;
        if(operation_bitmap[turn] == 1)
            continue;
        else{
            operation_bitmap[turn] = 1;
            loop_index++;
        }}

    char buffer[12];
    unsigned int key, seed;
    gen_seed(&seed);
    char * value;
    for(loop_index = 0; loop_index<elements; loop_index++){
        sprintf(buffer,"%u",insert_elements[loop_index]);
        MurmurHash3_x86_32(buffer,strlen(buffer),seed,&key);
        value = chunk_alloc(size);
        memset(value,'x',size);
        insert_hashtable(key,value,size);
    }
    
    char *pp = (char*) malloc(10*LLC_SIZE);
    memset(pp,0,10*LLC_SIZE);
    srand(37);
    unsigned int operation_index;
    unsigned int total_time = 0;
    for(loop_index = 0; loop_index < operations; loop_index++){
        
        operation_index = rand()%elements;
        sprintf(buffer,"%u",insert_elements[operation_index]);
        MurmurHash3_x86_32(buffer,strlen(buffer),seed,&key);
        RDTSC_START();
        if(operation_bitmap[loop_index]){
            access_id(key);
        }
        else{
            peek(key);
        }
        RDTSC_STOP();
        ELAPSED_TIME(start_hi,start_lo,end_hi,end_lo);
        total_time += time_taken;
    }
    //PRINT_LOG();
    RDTSC_START();
    TX_END();
    RDTSC_STOP();
    ELAPSED_TIME(start_hi,start_lo,end_hi,end_lo);
    total_time += time_taken;
    printf("Total Time: %u\n",total_time);

    munmap(operation_bitmap,operations*sizeof(char));
    munmap(master_bitmap,5*elements*sizeof(char));
    munmap(insert_elements,elements*sizeof(unsigned int));
    munmap(hash_table,HASH_SIZE*sizeof(struct hash_entry));
    munmap(hash_table_elements,(elements-HASH_SIZE)*sizeof(struct hash_entry));
    munmap(list_elements,elements*sizeof(struct element));
    munmap(elem_value_chunk,3*elements*size*sizeof(char));
    munmap(log_entries,elements*sizeof(struct log_entry));
    return 0;
}

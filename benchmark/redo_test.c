#include "redo.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct data{
    void * value;
};
int main(int argc, char* argv[]){
   
    int a = 1;
    int * b = &a;
    con_mech = 0;
    struct data d;
    struct data *d_p;
    d.value = malloc(128*sizeof(char));
    memset(d.value,'v',128*sizeof(char));
    d_p = &d;
    char de = 'k';
    //unsigned long address;
    //address = (unsigned long)&a;
    //printf("%lx %d\n",address,*(int*)address);
   // RDTSC_START();
    //INSERT_LOG(b,sizeof(int)); 
    //INSERT_LOG(b,sizeof(int)); 
    //INSERT_LOG(b,sizeof(int));
    void * value = malloc(sizeof(char)*128);
    memset(value,'z',sizeof(char)*128);
    INSERT_LOG((d.value),value,(sizeof(char)*128));
    char c;
    READ(d.value,&c,sizeof(char));
    printf("character: %c\n",c);
    READ(&de,&c,sizeof(char));
    printf("character: %c\n",c);
    /*struct log_entry * elm = (struct log_entry *)malloc(sizeof(struct log_entry ));
    (elm)->address = ((unsigned long)&a);
    (elm)->value = (malloc(sizeof(int)));
    memcpy((elm)->value,&a,sizeof(int));
    (elm)->next = NULL;
    */
    PRINT_LOG();
    TX_END();
    printf("character: %c\n",*(char*)(d.value));
    //RDTSC_STOP();
    //ELAPSED_TIME(start_hi,start_lo,end_hi,end_lo);
    //printf("%lu %u\n",time_taken, con_mech);

}

/***
  This is a simple REDO LOG implementation.
  REDO LOG is implemented as Queue data structure.
  LOG is flushed at transaction commit.
  TX_END applies the REDO Log
  **/

#include<stdint.h>
#include<string.h>

#define CACHE_LINE (1<<6)

struct log_entry{
    unsigned long address;
    char * value;
    unsigned int size;
    struct log_entry *next;
};

struct log{
    struct log_entry *head;
    struct log_entry *tail;
};

struct log redo_log = {NULL,NULL};
struct log_entry * elm;

void * fl_addr;
unsigned int con_mech;

#define FLUSH_LINE(fl_addr)\
    do{ \
       asm volatile("sfence");\
       if(con_mech == 1){\
            asm volatile ("clflush (%0)\n"\
            :: "c" (fl_addr)\
            : "rax");}\
        else if(con_mech == 2){\
            asm volatile ("clflushopt (%0)\n"\
            :: "c" (fl_addr)\
            : "rax");}\
        else if(con_mech == 3){\
	    /*printf("flush addr: %p\n",fl_addr);*/\
            asm volatile ("clwb (%0)\n"\
            :: "c" (fl_addr)\
            : "rax");}\
        asm volatile("sfence");}\
    while(0)

unsigned int num, loop_index;
#define FLUSH_MULTI_LINE(addr, size)\
    do{ \
        num = (size>>6);\
	/*printf("addr: %p num: %u\n",addr,num);*/\
        for(loop_index=0; loop_index<=num; loop_index++){\
            FLUSH_LINE((void *)((unsigned long)addr&(~(CACHE_LINE-1)))+(loop_index*(1<<6)));}}\
    while(0)

struct log_entry * log_entries;

#define CREATE_LOG_ENTRY(addr,valueptr,vsize)\
    do{ \
        elm = (struct log_entry *)log_entry_alloc();\
        (elm)->address = ((unsigned long)addr);\
        (elm)->value = chunk_alloc(vsize);\
        memcpy((elm)->value,valueptr,vsize);\
        (elm)->size = vsize;\
        (elm)->next = NULL;}\
    while(0)


#define INSERT_LOG(addr,valueptr,size) \
    do{ \
        CREATE_LOG_ENTRY(addr,valueptr,size);\
        if((redo_log).head == NULL){\
            (redo_log).head = (elm);\
            (redo_log).tail = (elm);}\
        else{\
            ((redo_log).tail)->next = (elm);\
            (redo_log).tail = (elm);}}\
    while(0)

#define TX_COMMIT()\
    do{\
        FLUSH_MULTI_LINE(&redo_log, sizeof(struct log));\
        struct log_entry *temp1;\
        temp1 = (redo_log).head;\
        for(; temp1; temp1 = temp1->next){\
	    /*printf("temp1:%p temp1->next:%p\n",temp1,temp1->next);*/\
            FLUSH_MULTI_LINE(temp1, sizeof(struct log_entry));\
	    /*printf("address:%p size:%u\n",temp1->value,temp1->size);*/\
            FLUSH_MULTI_LINE(temp1->value, temp1->size);}}\
    while(0)

#define TX_END()\
    do{\
        TX_COMMIT();\
        struct log_entry *temp2;\
        temp2 = (redo_log).head;\
        for(; temp2; temp2 = temp2->next){\
            memcpy((void*)temp2->address,temp2->value,temp2->size);}}\
    while(0);

/*
void tx_end(){
    FLUSH_MULTI_LINE(&redo_log, sizeof(struct log));
    struct log_entry *temp1,*temp2;
    temp1 = (redo_log).head;
    for(; temp1; temp1 = temp1->next){
        printf("temp1:%p temp1->next:%p\n",temp1,temp1->next);
        FLUSH_MULTI_LINE(temp1, sizeof(struct log_entry));
	printf("address:%p size:%u\n",temp1->value,temp1->size);
        FLUSH_MULTI_LINE(temp1->value, temp1->size);
    }
    temp2 = (redo_log).head;
    for(; temp2; temp2 = temp2->next){
        memcpy((void*)temp2->address,temp2->value,temp2->size);
    }
}
*/
#define READ(addr,destptr,size)\
    do{\
        struct log_entry *temp3;\
        char flag = 'n';\
        temp3 = (redo_log).head;\
        for(; temp3; temp3=temp3->next){\
            if(temp3->address == (unsigned long)addr){\
                flag = 'y';\
                memcpy(destptr,temp3->value,size);}\
            break;}\
        if(flag == 'n'){\
            memcpy(destptr,addr,size);}}\
    while(0);



#define PRINT_LOG()\
    do{ \
        struct log_entry * temp;\
        (temp) = (redo_log).head;\
        for(;temp; temp=temp->next){\
            fprintf(stdout,"temp:%p value:%p size:%u\n",temp,(temp->value),temp->size);}}\
    while(0)

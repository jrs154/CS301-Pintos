// // /* Tests producer/consumer communication with different numbers of threads.
// //  * Automatic checks only catch severe problems like crashes.
// //  */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#define SIZE 12


char* inp = "HELLO_WORLD";
char value = -1;
//int fp[MAXPC], fc[MAXPC];
void producer_consumer(unsigned int num_producer, unsigned int num_consumer);

typedef char queue_t;
queue_t buffer[SIZE];
int index; 
int max_buffer;
struct lock buffer_lock;
struct condition full_cond ;
struct condition empty_cond;


void pushqueue(queue_t value) {
    if (index < SIZE) {
        buffer[index] = value;
        index++;
    } else {
        printf("Buffer overflow\n");
    }
    max_buffer++;
}
 
queue_t popqueue() {
    char x;
    if (index > 0) 
    {
        x = buffer[max_buffer - index];
        //printf("\n%c\n", x);
        index--;
        return x;
    }
    else {
        printf("Buffer underflow\n");
        return '\0';
    }
    
}
 
int isempty() {
    if (index == 0)
        return 1;
    return 0;
}
 
int isfull() {
    if (index == SIZE)
        return 1;
    return 0;
}
 
void *producer(void *thread_n) {
    int i = 0;
    int thread_numb = *(int *)thread_n; 
    queue_t val = inp[++value];
    //timer_sleep(10);
    lock_acquire(&buffer_lock);

    while (isfull()) 
    {
        printf("Buffer is full");
        cond_wait(&full_cond, &buffer_lock);
    }
    if (isempty()) {
        pushqueue(val);
        cond_signal(&empty_cond, &buffer_lock);
    } else {
        pushqueue(val);
    }
    printf("Producer thread %d inserted %c\n", thread_numb, inp[value]);
    // for(i=0; i<index; i++)
    //     printf("%c ", buffer[i]);
    // printf("\n");
    lock_release(&buffer_lock);    
    thread_exit();
    
}
 
void *consumer(void *thread_n) {
    int i = 0;
    queue_t val;
    int thread_numb = *(int *)thread_n;
    timer_sleep(10);
    lock_acquire(&buffer_lock);

    while(isempty()) {
        printf("Buffer is empty");
        cond_wait(&empty_cond, &buffer_lock);
    }
    if (isfull()) {
        val = popqueue();
        cond_signal(&full_cond, &buffer_lock);
    } else {
        val = popqueue();
    }
    //printf("%c\n", val);
    printf("Consumer thread %d processed %c\n", thread_numb, val);
    // for(i=0; i<index; i++)
    //     printf("%c ", buffer[i]);
    // printf("\n");
    lock_release(&buffer_lock);
    thread_exit();
    
}

void test_producer_consumer(void)
{
    producer_consumer(0, 0);
    producer_consumer(1, 0);
    producer_consumer(0, 1);
    producer_consumer(1, 1);
    producer_consumer(3, 1);
    producer_consumer(1, 3);
    producer_consumer(4, 4);
    producer_consumer(7, 2);
    producer_consumer(2, 7);
    producer_consumer(7, 7);
    pass();
}

int sumarr(int n, int *arr)
{
    int res = 0;
    for(int i=0; i<n; i++)
        res += arr[i];
    return res;
}

void producer_consumer(UNUSED unsigned int num_producer, UNUSED unsigned int num_consumer)
{
    lock_init(&buffer_lock);
    cond_init(&full_cond);
    cond_init(&empty_cond);
    index = 0;
    max_buffer = 0;
    memset(buffer, '\0', sizeof(char)*SIZE);
    value = -1;
    int i;
    struct thread *pro_thread[num_producer], *con_thread[num_consumer];
    int p_num[num_producer], c_num[num_consumer];

    for (i = 0; i < num_producer; ) 
    {
        p_num[i] = i;
        
        thread_create(&pro_thread[i],   
                       NULL,         
                       producer,     
                       &p_num[i]);   
        i++;
    }
    
    for (i = 0; i < num_consumer; ) 
    {
        c_num[i] = i;
        
        thread_create(&con_thread[i],
                       NULL,
                       consumer,
                       &c_num[i]);
        i++;
    }
    
 }


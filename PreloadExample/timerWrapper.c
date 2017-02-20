#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

struct timespec startTime, endTime;

void startTimer(void){
    clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);
}

uint64_t getTimer(void){
    clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
    uint64_t timeElapsed = (endTime.tv_sec - startTime.tv_sec)*1000000;
    timeElapsed += (endTime.tv_nsec - startTime.tv_nsec) / 1000;
    return timeElapsed;
}

//typedef int (*original_printf_type) (const char *format, ...);
//typeof(puts) *original_puts_type;
typedef int (*original_puts_type)(const char *str);

int puts(const char *str){
    printf("Begin timer for puts...\n");
   // startTimer();

    
   // original_puts_type original_puts;
   // original_puts = (original_puts_type)dlsym(RTLD_NEXT,"puts");
   // original_puts(str);

   // uint64_t timeElapsed = getTimer();

   // printf("puts took %d microseconds\n", timeElapsed);
   return 1;
}


//template for an overridden function
/* void overriddenFunction(void){
    printf("Begin timer for function override #1...\n");
    startTimer();
    //call original function
    //originalFunction();
    uint64_t timeElapsed = getTimer();
    printf("Function override #1 took %d microseconds\n", timeElapsed);
}
*/
//test main for testing the startTimer and getTimer functions
/*
void main(){
    startTimer();
    int i = 0;
    while(i<2000000000){
        i++;
    //printf("waiting ");
    }
    uint64_t x = getTimer();
    printf("took %d microseconds\n", x);
}
*/

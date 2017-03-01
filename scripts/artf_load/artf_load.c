
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>

/******** SETTINGS *********/
// Time data points at 10MB
#define DEFAULT_TIMEPOINTS (10e6)
// Memory load in bytes - 1500MB
#define DEFAULT_MEMORY_LOAD (1500e6)
// Resolution of timing measurements
// Decrease for FASTER/more accurate time data
#define TIMING_RESOLUTION (100)
/******** SETTINGS *********/


// 8GB max
#define MAX_MEMORY_LOAD (8000e6)
// 10MB min
#define MIN_MEMORY_LOAD (10e6)
// Large random negative number for marking not set data points
#define NO_TIME_SET ((long int)0xFFFFFFFFFFFFFFFF)

int8_t* mem;
clock_t* timeData;
// Number of potential time measurements
int32_t timePoints = DEFAULT_TIMEPOINTS;
int32_t memoryLoad = DEFAULT_MEMORY_LOAD;

// Used for debugging
void calculateAvg() {
    int64_t avg = 0, count = 0, i;

    // find the largest gap between the time entries in the time dat
    for (i = 1; i < timePoints; i++) {
        // End if this is a data point that was never set
        if (timeData[i] == NO_TIME_SET) {
            break;
        }

        avg += (int64_t)timeData[i] - (int64_t)timeData[i-1];
        count++;
    }

    // TODO: remove
    avg /= count;
    printf("%ld\n", count);

    double t = ((double)avg)/CLOCKS_PER_SEC;
    printf("\nthe avg was: %.8f\n", t);
}

// Used for debugging
void calculateMin() {
    int32_t smallestDowntime = INT_MAX, i;

    // find the largest gap between the time entries in the time dat
    for (i = 1; i < timePoints; i++) {
        int gap = timeData[i] - timeData[i-1];

        // Account for the case where the clock has wrapped around int max
        if (timeData[i] < timeData[i-1]) {
           gap = timeData[i] - INT_MIN;
           gap += INT_MAX - timeData[i-1];
        }

        // End if this is a data point that was never set
        if (timeData[i] == NO_TIME_SET) {
            break;
        }
        
        if (gap < smallestDowntime) {
            smallestDowntime = gap;
        }
    }

    double t = ((double)smallestDowntime)/CLOCKS_PER_SEC;
    printf("\nthe smallest measured downtime was: %.8f\n", t);
}

void calculateDowntime() {
    clock_t largestDowntime = INT_MIN;
    int i;

    // find the largest gap between the time entries in the time dat
    for (i = 1; i < timePoints; i++) {
        clock_t gap = timeData[i] - timeData[i-1];

        // End if this is a data point that was never set
        if (timeData[i] == NO_TIME_SET) {
            break;
        }
        
        if (gap > largestDowntime) {
            largestDowntime = gap;
        }
    }

    double t = ((double)largestDowntime)/CLOCKS_PER_SEC;
    printf("\nThe largest measured downtime was:   %.8f sec\n", t);
}

void sigHandler(int signo) {
    if (signo == SIGINT) {
        calculateDowntime();
        exit(0);
    }
}

void init(int argc, const char* argv[], struct sigaction* psa) {
    time_t t;
    
    // Seed random
    srand((int) time(&t));

    // Process user input
    if (argc > 1) {
        memoryLoad = atoi(argv[1]) * 1000000;
        if (memoryLoad > MAX_MEMORY_LOAD || memoryLoad < MIN_MEMORY_LOAD) {
            printf("Input memory load is too high");
            exit(1);
        }
    }

    // Allocate memory to 0
    mem = malloc(sizeof(int8_t) * memoryLoad + 1);
    timeData = malloc(sizeof(clock_t) * timePoints);

    
    // This also allows the program to quickly hit every memory locations, as the load function will take
    // 10 or so seconds to hit every location
    memset(mem, 0, sizeof(int8_t) * memoryLoad);
    // Set NO_TIME_SET flag to indicate the end of the recorded time data
    memset(timeData, NO_TIME_SET, sizeof(clock_t) * timePoints);
    
    // Initialize exit signal handler
    psa->sa_handler = sigHandler;
    sigaction(SIGINT, psa, NULL);
}

void load() {
    int i = 0, timeIdx = 0;

    while(1) {
        // Access indexed memory locations to simulate a memory load
        // The random access is to attempt to simulate a more realistic load
        mem[i] = mem[ rand()%memoryLoad ] + 1;

        // Every X'th iteration (determined by TIMING_RESOLUTION) add a time data point
        if ( (i % TIMING_RESOLUTION) == (TIMING_RESOLUTION - 1) ) {
            timeData[timeIdx] = clock();
            timeIdx++;

            // Out of time data points
            // Exit and show the largest downtime found 
            if (timeIdx >= timePoints) {
                printf("Time measurement was running too long.\n");
                printf("Increase the number of time data points for a longer runtime.\n");

                calculateDowntime();
                exit(1);
            }
        }

        i = (i + 1) % memoryLoad;
    }
}


int main(int argc, const char* argv[]) {
    struct sigaction psa;

    memset(&psa, 0, sizeof(psa));

    init(argc, argv, &psa);

    printf("System is underload and timing downtimes\n");
    printf("Press ctrl-c to quit\n");
    fflush(stdout);

    // Put load on memory and measure time
    load();
}


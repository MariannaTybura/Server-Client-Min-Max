#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>

#define SERVER_MEMORY_NAME "server_memory_name"
#define SERVER_SEMAPHORE_NAME "server_semaphore_name"

struct server_memory_t {
    sem_t sem2;
    sem_t sem3;
    pid_t server_pid;
    char client_memory_name[100];
    int32_t min;
    int32_t max;
    size_t number_of_numbers;
};

#endif //COMMON_H

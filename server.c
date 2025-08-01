#include "common.h"

size_t stats = 0;
size_t total_number_of_numbers = 0;
float avg = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

int end_of_working = 0;
pthread_mutex_t end_of_working_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t * server_is_ready_sem;

void * process_data(void * arg) {
    struct server_memory_t * server_memory = (struct server_memory_t *) arg;

    while(1) {
        sem_wait(&server_memory->sem2);

        pthread_mutex_lock(&end_of_working_mutex);
        if(end_of_working == 1) {
            pthread_mutex_unlock(&end_of_working_mutex);
            break;
        }
        pthread_mutex_unlock(&end_of_working_mutex);

        int fd_client = shm_open(server_memory->client_memory_name, O_RDWR, 0777);
        if(fd_client == -1) {
            printf("Error while opening client's shared memory\n");
            sem_post(&server_memory->sem3);
            continue;
        }
        int32_t * tab = mmap(NULL, server_memory->number_of_numbers * sizeof(int32_t), PROT_READ, MAP_SHARED, fd_client, 0);
        if(tab == MAP_FAILED) {
            printf("Error while mapping client's shared memory");
            sem_post(&server_memory->sem3);
            continue;
        }
        server_memory->min = INT_MAX;
        server_memory->max = INT_MIN;

        pthread_mutex_lock(&stats_mutex);
        stats++;
        total_number_of_numbers = total_number_of_numbers + server_memory->number_of_numbers;
        avg = (float)total_number_of_numbers / (float)stats;
        pthread_mutex_unlock(&stats_mutex);

        for(int i = 0 ; i < server_memory->number_of_numbers; i++) {
            if(*(tab + i) > server_memory->max) {
                server_memory->max = *(tab + i);
            }
            if(*(tab + i) < server_memory->min) {
                server_memory->min = *(tab + i);
            }
        }
        munmap(tab, server_memory->number_of_numbers * sizeof(int32_t));
        close(fd_client);

        sem_post(&server_memory->sem3);
    }
    return NULL;
}

void * user_interface(void * arg) {
    struct server_memory_t * server_memory = (struct server_memory_t *) arg;
    while(1) {
        char buffor[6];
        scanf("%5s", buffor);
        while(getchar() != '\n');
        if(strcmp(buffor, "quit") == 0) {
            pthread_mutex_lock(&end_of_working_mutex);
            end_of_working = 1;
            pthread_mutex_unlock(&end_of_working_mutex);
            sem_wait(server_is_ready_sem);
            sem_post(&server_memory->sem2);
            break;
        }
        else if(strcmp(buffor, "reset") == 0) {
            pthread_mutex_lock(&stats_mutex);
            stats = 0;
            total_number_of_numbers = 0;
            avg = 0;
            pthread_mutex_unlock(&stats_mutex);
        }
        else if(strcmp(buffor, "stats") == 0) {
            pthread_mutex_lock(&stats_mutex);
            printf("Stats: %zu\n", stats);
            printf("Avg: %f\n", avg);
            pthread_mutex_unlock(&stats_mutex);
        }
        else {
            printf("Command not found\n");
        }
    }
    return NULL;
}

int main() {
    int fd = shm_open(SERVER_MEMORY_NAME, O_CREAT | O_TRUNC | O_RDWR, 0777);
    if(fd == -1) {
        printf("Error while opening server's shared memory");
        return 1;
    }

    int result = ftruncate(fd, sizeof(struct server_memory_t));
    if(result == -1) {
        printf("Error during truncating");
        close(fd);
        shm_unlink(SERVER_MEMORY_NAME);
        return 1;
    }

    struct server_memory_t * server_memory = mmap(NULL, sizeof(struct server_memory_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(server_memory == MAP_FAILED) {
        printf("Error during mapping");
        close(fd);
        shm_unlink(SERVER_MEMORY_NAME);
        return 1;
    }

    sem_unlink(SERVER_SEMAPHORE_NAME);
    server_is_ready_sem = sem_open(SERVER_SEMAPHORE_NAME, O_CREAT | O_RDWR, 0777, 0);
    if(server_is_ready_sem == SEM_FAILED) {
        printf("Error while opening server_is_ready_sem");
        munmap(server_memory, sizeof(struct server_memory_t));
        close(fd);
        shm_unlink(SERVER_MEMORY_NAME);
        return 1;
    }

    if(sem_init(&server_memory->sem2, 1, 0) != 0) {
        printf("Error while initializing sem2");
        sem_close(server_is_ready_sem);
        sem_unlink(SERVER_SEMAPHORE_NAME);
        munmap(server_memory, sizeof(struct server_memory_t));
        close(fd);
        shm_unlink(SERVER_MEMORY_NAME);
        return 1;
    }
    if(sem_init(&server_memory->sem3, 1, 0) != 0) {
        printf("Error while initializing sem3");
        sem_close(server_is_ready_sem);
        sem_unlink(SERVER_SEMAPHORE_NAME);
        sem_destroy(&server_memory->sem2);
        munmap(server_memory, sizeof(struct server_memory_t));
        close(fd);
        shm_unlink(SERVER_MEMORY_NAME);
        return 1;
    }

    server_memory->server_pid = getpid();

    pthread_t process_data_thread;
    pthread_t user_interface_thread;

    pthread_create(&process_data_thread, NULL, process_data, server_memory);
    pthread_create(&user_interface_thread, NULL, user_interface, server_memory);

    sem_post(server_is_ready_sem);

    pthread_join(process_data_thread, NULL);
    pthread_join(user_interface_thread, NULL);

    sem_close(server_is_ready_sem);
    sem_unlink(SERVER_SEMAPHORE_NAME);

    sem_destroy(&server_memory->sem2);
    sem_destroy(&server_memory->sem3);
    munmap(server_memory, sizeof(struct server_memory_t));
    close(fd);
    shm_unlink(SERVER_MEMORY_NAME);
    return 0;
}

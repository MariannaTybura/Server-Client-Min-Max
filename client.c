#include "common.h"

int main(int argc, char * argv[]) {
    if(argc != 2) {
        printf("Wrong number of arguments");
        return 1;
    }
    FILE * file = fopen(argv[1], "r");
    if(file == NULL) {
        printf("Couldn't open file");
        return 1;
    }

    sem_t * server_is_ready_sem = sem_open(SERVER_SEMAPHORE_NAME, O_RDWR, 0777, 0);
    if(server_is_ready_sem == SEM_FAILED) {
        printf("Error while opening server_is_ready_sem, server is not working");
        fclose(file);
        return 1;
    }

    size_t number_of_numbers = 0;
    while(!feof(file)) {
        int32_t number;
        if(fscanf(file, "%d", &number) != 1) {
            printf("File corrupted, there must be only numbers\n");
            fclose(file);
            sem_close(server_is_ready_sem);
            return 1;
        }
        number_of_numbers++;
    }
    fseek(file, 0, SEEK_SET);

    printf("Number of found numbers: %zu\n", number_of_numbers);

    char client_memory_name[30];
    sprintf(client_memory_name, "%d_client_memory_name", getpid());

    int fd = shm_open(client_memory_name, O_CREAT | O_TRUNC | O_RDWR, 0777);
    if(fd == -1) {
        printf("Error while opening client's shared memory");
        fclose(file);
        sem_close(server_is_ready_sem);
        return 1;
    }

    int result = ftruncate(fd, number_of_numbers * sizeof(int32_t));
    if(result == -1) {
        printf("Error while truncating");
        fclose(file);
        sem_close(server_is_ready_sem);
        close(fd);
        shm_unlink(client_memory_name);
        return 1;
    }

    int32_t * tab = mmap(NULL, sizeof(int32_t) * number_of_numbers, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(tab == MAP_FAILED) {
        printf("Error while mapping");
        fclose(file);
        sem_close(server_is_ready_sem);
        close(fd);
        shm_unlink(client_memory_name);
        return 1;
    }

    for(int i = 0; i < number_of_numbers; i++) {
        int32_t temp;
        fscanf(file, "%d", &temp);
        *(tab + i) = temp;
    }

    fclose(file);

    int fd_server = shm_open(SERVER_MEMORY_NAME, O_RDWR, 0777);
    if(fd_server == -1) {
        printf("Error while opening server's shared memory");
        sem_close(server_is_ready_sem);
        munmap(tab, number_of_numbers * sizeof(int32_t));
        close(fd);
        shm_unlink(client_memory_name);
        return 1;
    }
    struct server_memory_t * server_memory = mmap(NULL, sizeof(struct server_memory_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd_server, 0);
    if(server_memory == MAP_FAILED) {
        printf("Error during mapping");
        sem_close(server_is_ready_sem);
        munmap(tab, number_of_numbers * sizeof(int32_t));
        close(fd);
        shm_unlink(client_memory_name);
        close(fd_server);
        return 1;
    }

    if(kill(server_memory->server_pid, 0) != 0) {
        printf("Server is not working, but semaphore is found");
        sem_close(server_is_ready_sem);
        munmap(tab, number_of_numbers * sizeof(int32_t));
        close(fd);
        shm_unlink(client_memory_name);
        munmap(server_memory, sizeof(struct server_memory_t));
        close(fd_server);
        return 1;
    }

    sem_wait(server_is_ready_sem);

    strcpy(server_memory->client_memory_name, client_memory_name);
    server_memory->number_of_numbers = number_of_numbers;

    sem_post(&server_memory->sem2);
    sem_wait(&server_memory->sem3);

    printf("Min: %d\n", server_memory->min);
    printf("Max: %d\n", server_memory->max);

    munmap(server_memory, sizeof(struct server_memory_t));
    close(fd_server);

    sem_post(server_is_ready_sem);

    sem_close(server_is_ready_sem);
    munmap(tab, number_of_numbers * sizeof(int32_t));
    close(fd);
    shm_unlink(client_memory_name);
    return 0;
}

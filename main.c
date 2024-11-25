#define BUFFER_SIZE 15
#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>

// Shared circular buffer and semaphores
char buffer[BUFFER_SIZE];
int in = 0, out = 0;
sem_t sem_empty, sem_full, sem_mutex;

void *producer(void *arg)
{
    FILE *fp = fopen("mytest.dat", "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    char newChar;
    while (fscanf(fp, "%c", &newChar) != EOF)
    {
        sem_wait(&sem_empty); // Wait for an empty slot
        sem_wait(&sem_mutex); // Lock the buffer

        buffer[in] = newChar;
        in = (in + 1) % BUFFER_SIZE; // Move to the next slot

        sem_post(&sem_mutex); // Unlock the buffer
        sem_post(&sem_full);  // Signal a filled slot
    }

    // Write special character to indicate EOF
    sem_wait(&sem_empty);
    sem_wait(&sem_mutex);

    buffer[in] = '*';
    in = (in + 1) % BUFFER_SIZE;

    sem_post(&sem_mutex);
    sem_post(&sem_full);

    fclose(fp);
    return NULL;
}

void *consumer(void *arg)
{
    char newChar;
    do
    {
        sem_wait(&sem_full);  // Wait for a filled slot
        sem_wait(&sem_mutex); // Lock the buffer

        newChar = buffer[out];
        out = (out + 1) % BUFFER_SIZE; // Move to the next slot

        sem_post(&sem_mutex); // Unlock the buffer
        sem_post(&sem_empty); // Signal an empty slot

        if (newChar != '*')
        {
            printf("%c", newChar);
            fflush(stdout);
        }

        sleep(1); // Slow down the consumer
    } while (newChar != '*');

    return NULL;
}

int main()
{
    pthread_t producer_thread, consumer_thread;

    // Initialize semaphores
    sem_init(&sem_empty, 0, BUFFER_SIZE); // BUFFER_SIZE empty slots
    sem_init(&sem_full, 0, 0);            // No filled slots initially
    sem_init(&sem_mutex, 0, 1);           // Binary semaphore for mutual exclusion

    // Create threads
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    // Wait for threads to finish
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    // Destroy semaphores
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    sem_destroy(&sem_mutex);

    return 0;
}

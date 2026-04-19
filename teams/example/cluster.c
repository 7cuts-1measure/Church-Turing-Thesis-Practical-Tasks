#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

uint32_t max_capacity, numWorkers = 5, numConsumers;

pthread_mutex_t nodes[5]; // узлы кластера
int worker_scores[5]; // счётчики сообщений, которые передали worker'ы


void transfer_data(int p) {
	pthread_mutex_lock(&nodes[p]);
    pthread_mutex_lock(&nodes[(p+1)%5]);

    worker_scores[p]++;

    pthread_mutex_unlock(&nodes[p]);
    pthread_mutex_unlock(&nodes[(p+1)%5]);
}


void *worker (void * arg) {

	uint32_t id = *((uint32_t *) arg);
	long report = 0;

	while(1) {
		transfer_data(id);
        printf("transferred\n");
	}
    return NULL;
}



int main(int argc, char * argv[]) {
    printf("hello from %d\n", getpid());
	srand(42);

    for (int i = 0; i < 5; i++){
        pthread_mutex_init(&nodes[i], NULL);
    }
    pthread_t workers[5];
    uint32_t threadIds[5];

	/* Create the workers */
	for (int i = 0; i < numWorkers ; i++) {
        threadIds[i] = i;
		int err = pthread_create(&workers[i], NULL, worker, &threadIds[i]);
        if (err == EAGAIN) {
            fprintf(stderr, "Insufficient resources to create another thread.\n");
            return -1;
        } else if (err == EINVAL) {
            fprintf(stderr, "Incorrect attribute was used.\n");
            return -1;
        } else if (err == EPERM) {
            fprintf(stderr, "No permission to set the scheduling policy and parameters specified in attribute\n");
            return -1;
        }

    }

	for (int i = 0; i < numWorkers ; i++)
		pthread_join(workers[i], NULL);

    pthread_exit(0);
}

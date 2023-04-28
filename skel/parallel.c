#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "os_list.h"

#define MAX_TASK 100
#define MAX_THREAD 4

int sum = 0;
os_graph_t *graph;
pthread_mutex_t sum_lock;
pthread_mutex_t graph_lock;
os_threadpool_t *th;
int is_graph_traversed = 0;

void processNode(void *arg) {
    os_node_t *node = (os_node_t *)arg;

    pthread_mutex_lock(&sum_lock);
    sum += node->nodeInfo; 
    pthread_mutex_unlock(&sum_lock);

    pthread_mutex_lock(&graph_lock);    
    for (int i = 0; i < node->cNeighbours; i++) {
        if (graph->visited[node->neighbours[i]] == 0) {
            graph->visited[node->neighbours[i]] = 1;
            add_task_in_queue(th, task_create((void *) graph->nodes[node->neighbours[i]], &processNode));
        }
    }
    pthread_mutex_unlock(&graph_lock);
}

void traverse_graph()
{
    for (int i = 0; i < graph->nCount; i++)
    {
        pthread_mutex_lock(&graph_lock);
        if (graph->visited[i] == 0) {
            graph->visited[i] = 1;
            os_task_t *tt = task_create((void *) graph->nodes[i], &processNode);
            add_task_in_queue(th, tt);
        }
        pthread_mutex_unlock(&graph_lock);
    }
    is_graph_traversed = 1;
}

int check_queue_empty(os_threadpool_t *tp) {
    if (tp->tasks == NULL && is_graph_traversed) {
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./main input_file\n");
        exit(1);
    }

    FILE *input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        printf("[Error] Can't open file\n");
        return -1;
    }

    graph = create_graph_from_file(input_file);
    if (graph == NULL) {
        printf("[Error] Can't read the graph from file\n");
        return -1;
    }

    // TODO: create thread pool and traverse the graf
    pthread_mutex_init(&sum_lock, NULL);
    pthread_mutex_init(&graph_lock, NULL);
    th = threadpool_create(MAX_TASK, MAX_THREAD);
    traverse_graph();
    threadpool_stop(th, &check_queue_empty);  
    printf("%d\n", sum);
    return 0;
}

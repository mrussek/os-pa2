#include "queue.h"
#include "util.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

/*
 * CSCI 3753
 * Programming Assignment 2
 * 
 * Group Members:
 * 
 * 1. Maxwell Russek
 * 2. Samuel Horton
 * 
 * */

/* Global queue for hostnames */
queue address_queue;

const int NUM_RESOLVERS = 2;
const int QUEUE_SIZE = 10;

void *producer(void *argument) {
	/* Read filename given in argument */
	FILE *input;
	char *line = NULL;
	int ret;
	size_t size;
	printf("%s\n", (char*)argument);
	input = fopen((char*)argument, "r");
	if (input == NULL) {
		fprintf(stderr, "Could not open file");
		pthread_exit(NULL);
	}

	while (getline(&line, &size, input) != -1) {
		if (!queue_is_full(&address_queue)) {
			ret = queue_push(&address_queue, line);
			if (ret == QUEUE_FAILURE) {
				fprintf(stderr, "Couldn't push for queue");
			}
		}
		free(line);
		line = NULL;
	}
	fclose(input);
	pthread_exit(NULL);
}

void *consumer(void *argument) {
	/* Read from queue and resolve */
	FILE *output = (FILE*)argument;
	char *hostname;
	char ip[INET6_ADDRSTRLEN];
	int ret;
	while (!queue_is_empty(&address_queue)) {
		hostname = queue_pop(&address_queue);
		if (hostname != NULL) {
			ret = dnslookup(hostname, ip, sizeof(ip));
			if (ret == UTIL_FAILURE)
				fprintf(stderr, "DNS lookup failed");
			fprintf(output, "%s,%s\n", hostname, ip);
		}
	}
	pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
	pthread_t *requesters, *resolvers;
	FILE *output;
	int i;

	if (argc < 3) {
		fprintf(stderr, "Not enough enough arguments\n");
		return 1;
	}

	output = fopen((char*)argv[argc - 1], "w");
	requesters = malloc(sizeof(pthread_t) * argc - 2);
	resolvers = malloc(sizeof(pthread_t) * NUM_RESOLVERS);

	if (queue_init(&address_queue, QUEUE_SIZE) == QUEUE_FAILURE) {
		fprintf(stderr, "Could not create queue\n");
		return 1;
	}

	for (i = 1; i < (argc - 1); i++) {
		pthread_create(&requesters[i - 1], NULL, producer, argv[i]);
	}

	for (i = 0; i < NUM_RESOLVERS; i++) {
		pthread_create(&resolvers[i], NULL, consumer, output);
	}

	for (i = 0; i < (argc - 2); i++) {
		pthread_join(requesters[i], NULL);
	}

	for (i = 0; i < NUM_RESOLVERS; i++) {
		pthread_join(resolvers[i], NULL);
	}

	fclose(output);
	
	return 0;
}

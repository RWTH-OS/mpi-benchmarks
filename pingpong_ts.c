/*
 * Copyright 2017, Simon Pickartz Institute for Automation
 *                                of Complex Power Systems,
 *                                RWTH Aachen University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mpi.h>

#define _CACHE_WARM_UP_

#define MAXBUFSIZE (64)
#define DEFAULTLEN (0)
#define DEFAULTDELAY (1e6)
#define DEFAULTROUNDS (10000)
#define DEFAULTITER (1)
#define WARMUPITER (10000)

#define send_buffer buffer
#define recv_buffer buffer
unsigned char buffer[MAXBUFSIZE + 1];

unsigned char dummy = 0;

int main(int argc, char **argv) {
	int arg;
	uint32_t i;
	int32_t num_ranks;
	int32_t remote_rank, my_rank;

	useconds_t delay = DEFAULTDELAY;
	uint32_t length = DEFAULTLEN;
uint32_t iterations = DEFAULTITER;
int32_t numrounds = DEFAULTROUNDS;
int32_t round;

double timer;
bool run_infinitely;
MPI_Status status;

/* determine arguments */
while ((arg = getopt(argc, argv, "i:r:l:hd:")) != -1) {
	switch (arg) {
		case 'r':
			numrounds = atoi(optarg);
			break;
		case 'l':
			length = atoi(optarg);
			break;
		case 'i':
			iterations = atoi(optarg);
			break;
		case 'd':
			delay = atof(optarg);
			break;
		case 'h':
			printf(
			    "usage %s [-l message_length (def: %d)] "
			    "[-i iterations (def: %d)] "
			    "[-d delay in us (def: %f)] "
			    "[-r rounds (def: %d)]\n",
			    argv[0], DEFAULTLEN, DEFAULTITER, DEFAULTDELAY,
			    DEFAULTROUNDS);
			exit(0);
	}
}

/* initialize MPI environment */
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

/* check for errors and determine remote rank */
if (num_ranks != 2) {
	if (my_rank == 0)
		fprintf(stderr, "%s needs exactly two UEs; try again\n",
			argv[0]);
	exit(-1);
}
remote_rank = (my_rank + 1) % 2;

/* perform a warm-up of the cache */
#ifdef _CACHE_WARM_UP_
for (i = 0; i < length; i++) {
	/* cache warm-up: */
	dummy += send_buffer[i];
	dummy += recv_buffer[i];
}
#endif

/* check for infinite test */
if (numrounds == -1) {
	run_infinitely = true;
} else {
	run_infinitely = false;
}

if (my_rank == 0) {
	printf("Starting the benchmark:\n");
	if (numrounds == -1) {
		printf("Rounds     :        inf\n");
	} else {
		printf("Rounds     : %10d\n", numrounds);
	}
	printf("Iterations : %10d\n", iterations);
	printf("Msg Length : %10d\n", length);
}

/* synchronize and start the PingPong */
MPI_Barrier(MPI_COMM_WORLD);
if (my_rank == 0) {
	for (i = 0; i < WARMUPITER; ++i) {
		MPI_Send(send_buffer, length, MPI_CHAR, remote_rank, 0,
			 MPI_COMM_WORLD);
		MPI_Recv(recv_buffer, length, MPI_CHAR, remote_rank, 0,
			 MPI_COMM_WORLD, &status);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	for (round = 0; run_infinitely || (round < numrounds);
	     ++round) {
		/* start timer: */
		timer = MPI_Wtime();

		for (i = 0; i < iterations; ++i) {
			MPI_Send(send_buffer, length, MPI_CHAR,
				 remote_rank, 0, MPI_COMM_WORLD);
			MPI_Recv(recv_buffer, length, MPI_CHAR,
				 remote_rank, 0, MPI_COMM_WORLD,
				 &status);
		}

		/* stop timer: */
		timer = (MPI_Wtime() - timer);

		printf("%1.2lf\n",
		       timer / (2.0 * iterations) * 1000000);
		fflush(stdout);

		// delay next ping
		useconds_t timer_usec = (useconds_t)(timer *1e6);
		useconds_t sleep_time;
		if (delay < timer_usec) {
			sleep_time = 0;
		} else {
			sleep_time = delay - timer_usec;
		}

		usleep(sleep_time);
	}
} else {
	for (i = 0; i < WARMUPITER; ++i) {
		MPI_Recv(recv_buffer, length, MPI_CHAR, remote_rank, 0,
			 MPI_COMM_WORLD, &status);
		MPI_Send(send_buffer, length, MPI_CHAR, remote_rank, 0,
			 MPI_COMM_WORLD);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	for (round = 0; run_infinitely || (round < numrounds);
	     ++round) {
		for (i = 0; i < iterations; ++i) {
				MPI_Recv(recv_buffer, length, MPI_CHAR,
					 remote_rank, 0, MPI_COMM_WORLD,
					 &status);
				MPI_Send(send_buffer, length, MPI_CHAR,
					 remote_rank, 0, MPI_COMM_WORLD);
			}
		}
	}

	MPI_Finalize();

	return 0;
}

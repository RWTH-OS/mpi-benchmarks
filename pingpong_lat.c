/*
 * Copyright 2014, Simon Pickartz Institute for Automation
 *                                of Complex Power Systems,
 *                                RWTH Aachen University
 *
 * Based on pingpong_length by Carsten Clauss, Chair for Operating Systems
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

#include <stat_eval.h>

#undef _WATCH_DOG_
#define _CACHE_WARM_UP_
#undef _USE_SEPARATED_BUFFERS_
#undef _PRINT_INDIVIDUAL_RES_

#define MAXBUFSIZE (64)
#define DEFAULTLEN (0)
#define DEFAULTROUNDS (10000)
#define DEFAULTITER (1)
#define WARMUPITER (10000)

#ifdef _USE_SEPARATED_BUFFERS_
unsigned char send_buffer[MAXBUFSIZE + 1];
unsigned char recv_buffer[MAXBUFSIZE + 1];
#else
#define send_buffer buffer
#define recv_buffer buffer
unsigned char buffer[MAXBUFSIZE + 1];
#endif
unsigned char dummy = 0;

int main(int argc, char **argv) {
	int arg;
	uint32_t i;
	int32_t num_ranks;
	int32_t remote_rank, my_rank;

	uint32_t length = DEFAULTLEN;
	uint32_t iterations = DEFAULTITER;
	int32_t numrounds = DEFAULTROUNDS;
	int32_t round;

	double timer;
	double *time_stamps = NULL;
	stat_eval_t stat_eval;
	bool run_infinitely;
	MPI_Status status;
	char *filename = NULL;

	/* determine arguments */
	while ((arg = getopt(argc, argv, "i:r:l:hf:")) != -1) {
		switch (arg) {
			case 'r':
				numrounds = atoi(optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'l':
				length = atoi(optarg);
				break;
			case 'i':
				iterations = atoi(optarg);
				break;
			case 'h':
				printf(
				    "usage %s [-l message_length (def: %d)] "
				    "[-i iterations (def: %d)] "
				    "[-r rounds (def: %d)] "
				    "[-f filename]\n",
				    argv[0], DEFAULTLEN, DEFAULTITER,
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
			fprintf(stderr,
				"Pingpong needs exactly two UEs; try again\n");
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
		time_stamps = (double *)calloc(sizeof(double), numrounds);
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
		if (filename) {
			printf("Filename   : %s\n", filename);
		} else {
			printf("Filename   :     stdout\n");
		}
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
			if (run_infinitely == false)
				time_stamps[round] =
				    timer * 1e6 / (2 * iterations);
#ifdef _PRINT_INDIVIDUAL_RES_
			printf("%d\t\t%1.2lf\t\t%1.2lf\n", length,
			       timer / (2.0 * iterations) * 1000000,
			       (length / (timer / (2.0 * iterations))) /
				   (1024 * 1024));
			fflush(stdout);

#endif
#ifdef _WATCH_DOG_
			if (!(round % 100000)) printf("Round %d ...\n", round);
#endif
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

	/* Statistical evaluation */
	if ((my_rank == 0) && (run_infinitely == false)) {
		statistical_eval(time_stamps, numrounds, &stat_eval);

		/* print the results */
		FILE *output = stdout;
		if (filename) {
			output = fopen(filename, "w+");
		}
		print_statistics(&stat_eval, numrounds, output);
		if (filename) {
			fclose(output);
		}
	}

	MPI_Finalize();

	return 0;
}

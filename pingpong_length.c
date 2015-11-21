/* 
 * Copyright 2010 Carsten Clauss, Chair for Operating Systems,
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mpi.h>

#define  _CACHE_WARM_UP_
#undef  _USE_SEPARATED_BUFFERS_
#undef  _ERROR_CHECK_
#undef  _EXTENDED_ERROR_CHECK_

#define MAXBUFSIZE  (1024*1024*64)
#define SEND_MASK   0xff
#define RECV_MASK   0x00
#define DEFAULTLEN  MAXBUFSIZE
#define NUMROUNDS   1000
#define WARM_UP     100


#ifdef _USE_SEPARATED_BUFFERS_
unsigned char send_buffer[MAXBUFSIZE+1];
unsigned char recv_buffer[MAXBUFSIZE+1];
#else
#define send_buffer buffer
#define recv_buffer buffer
unsigned char buffer[MAXBUFSIZE+1];
#endif
unsigned char dummy = 0;

int main(int argc, char **argv)
{
	int i;
	int num_ranks;
	int remote_rank, my_rank;
	int numrounds = NUMROUNDS;
	int maxlen    = DEFAULTLEN;
	int length;
	int round;
	double timer = 0;

	MPI_Status status;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

	if(num_ranks != 2) {
		if(my_rank == 0) 
			fprintf(stderr, "Pingpong needs exactly two UEs; "
					"try again\n");
		exit(-1);
	}

	remote_rank = (my_rank + 1) % 2;

	printf("Rank: %d; PID: %d\n", my_rank, getpid());

#ifdef _EXTENDED_ERROR_CHECK_
			unsigned char my_mask, rem_mask;
			
			/* intialize the buffers */
			if (my_rank == 0) {
				my_mask = SEND_MASK;
				rem_mask = RECV_MASK;
				for (i=0; i<MAXBUFSIZE; ++i) {
					send_buffer[i] = my_mask;	
					recv_buffer[i] = 0x42;	
				}
			} else {
				my_mask = RECV_MASK;
				rem_mask = SEND_MASK;
				for (i=0; i<MAXBUFSIZE; ++i) {
					send_buffer[i] = my_mask;	
					recv_buffer[i] = 0x42;	
				}
			}
#endif

	if(my_rank == 0) 
		printf("#bytes\t\tusec\t\tMB/sec\n");

	if(my_rank == 0) {
		for(length=1; length <= maxlen; length*=2) {

#ifdef _CACHE_WARM_UP_
			for(i=0; i < length; i++) {
				/* cache warm-up: */
				dummy += send_buffer[i];
				dummy += recv_buffer[i];
			}
#endif

			/* synchronize before starting PING-PONG: */
			MPI_Barrier(MPI_COMM_WORLD);

			for(round=0; round<numrounds+WARM_UP; round++) {

#ifdef _ERROR_CHECK_
				for(i=0; i < length; i++) {
					send_buffer[i] = (i+length+round) % 127;
				}
#endif

				/* send PING: */
				MPI_Ssend(send_buffer, 
				    	  length, 
					  MPI_CHAR, 
					  remote_rank, 
					  0, 
					  MPI_COMM_WORLD);

				/* recv PONG: */
				MPI_Recv(recv_buffer, 
				    	 length, 
					 MPI_CHAR, 
					 remote_rank, 
					 0, 
					 MPI_COMM_WORLD, 
					 &status);

				/* start timer: */
				if (round == WARM_UP-1)
					timer = MPI_Wtime();

#ifdef _ERROR_CHECK_
				for(i=0; i < length; i++) {
					if(recv_buffer[i] != (i+length+round) % 127) {
						fprintf(stderr, 
						    	"ERROR: %d VS %d at %d\n", 
							recv_buffer[i], 
							(i+length+round)%127, 
							i);
						exit(-1);
					}
				}
#endif
			}

			/* stop timer: */
			timer = MPI_Wtime() - timer;

			printf("%d\t\t%1.2lf\t\t%1.2lf\n", 
			       length, 
			       timer/(2.0*numrounds)*1000000, 
			       (length/(timer/(2.0*numrounds)))/(1024*1024));
			fflush(stdout);
		}
	} else {
		for(length=1; length <= maxlen; length*=2) {

#ifdef _CACHE_WARM_UP_
			for(i=0; i < length; i++) {
				/* cache warm-up: */
				dummy += send_buffer[i];
				dummy += recv_buffer[i];
			}
#endif

			/* synchronize before starting PING-PONG: */
			MPI_Barrier(MPI_COMM_WORLD);

			for(round=0; round<numrounds+WARM_UP; round++) {

#ifdef _ERROR_CHECK_
				for(i=0; i < length; i++) {
					send_buffer[i] = (i+length+round) % 127;
				}
#endif

				/* recv PING: */
				MPI_Recv(recv_buffer, 
				    	 length, 
					 MPI_CHAR, 
					 remote_rank, 
					 0, 
					 MPI_COMM_WORLD, 
					 &status);

				/* send PONG: */
				MPI_Ssend(send_buffer, 
				    	  length, 
					  MPI_CHAR, 
					  remote_rank, 
					  0, 
					  MPI_COMM_WORLD);

#ifdef _ERROR_CHECK_
				for(i=0; i < length; i++) {
					if(recv_buffer[i] != (i+length+round) % 127) {
						fprintf(stderr, 
						    	"ERROR: %d VS %d at %d\n", 
							recv_buffer[i], 
							(i+length+round)%127, 
							i);
						exit(-1);
					}
				}
#endif
			}
		}
	}


#ifdef _EXTENDED_ERROR_CHECK_
	printf("Perform error check ...\n");
	for (i=0; i<MAXBUFSIZE; ++i) {
		if (recv_buffer[i] != rem_mask) {
			fprintf(stderr, 
				"got: buf[%u] = 0x%x\n"
				"expected: buf[%u] = 0x%x\n",
				i, 
				recv_buffer[i], 
				i, 
				rem_mask);
		}
	}
#endif
	MPI_Finalize();

	return 0;
}


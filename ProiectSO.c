#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
 
# define MAX_DOCTORS 2			// The total number of doctors
# define MAX_CLIENTS 10			// The maximum number of clients
# define MAX_WAITING_TIME 10		// The maximum time a consultation can be held for
# define CLIENT_GENERATING_TIME 10 	// Number of seconds that the clients will be randomly generated for.
# define MAX_TIME_BETWEEN_CLIENTS 3 	// Number of seconds that there can be at max between 2 different clients coming into the clinic
 
int available_doctors = MAX_DOCTORS;
int doctors[MAX_DOCTORS];		// Availability of all doctors ( 1 = Busy, 0 = Available )
int current_time = 0;
int threads_still_running = 0;
int total_waiting_time = 0;
pthread_mutex_t mtx;
 
void* time_pass(int count)
{
	for (int i=0;i<count;i++)
	{
		pthread_mutex_lock(&mtx);
		current_time ++;
		pthread_mutex_unlock(&mtx);
 
		sleep(1);
	}
}
 
void* increase_doctor_count(const int current_doctor_id,const int waiting_time,const int consultation_time)
{
	pthread_mutex_lock(&mtx);
 
	doctors[current_doctor_id] = 0;
	available_doctors++;
	printf("\n\nThe doctor with the ID %d is now free. Time: %d",current_doctor_id,current_time);
	printf("\nThe pacient waited for %d seconds before being consulted by the doctor.",waiting_time);
	printf("\nThe pacient was consulted for %d seconds.\n",consultation_time);
	total_waiting_time += waiting_time;

	pthread_mutex_unlock(&mtx);
 
	return NULL;
}
 
void* decrease_doctor_count(int *current_doctor_id,int *waiting_time)
{
	jump:
 
	if (available_doctors < 1)				// If all doctors are already busy with pacients, we wait
	{
		//printf("\nNot enough doctors available,waiting"); 
		sleep(1);
		(*waiting_time)++;
		goto jump;
	}
	else 
	{
		pthread_mutex_lock(&mtx);
 
		for (int i=0;i<MAX_DOCTORS;i++)
		{
			if (doctors[i] == 0)
			{
				printf("\nThe doctor with the ID %d is now busy, Time: %d\n",i,current_time);
				(*current_doctor_id) = i;
				doctors[i] = 1;		// We tell the doctor with the ID equal to i to take care of the current pacient
				available_doctors--;
				break;
			}
		}
 
		pthread_mutex_unlock(&mtx);
	}
 
	return NULL;
}
 
void* consultation()
{
	printf("\nA new client arrived, Time: %d",current_time);
	int current_doctor_id = -1;
	int *curr_doctor_id = &current_doctor_id;
	int waiting_time = 0;
	int *wait_time = &waiting_time;
 
	decrease_doctor_count(curr_doctor_id,wait_time);
 
	int consultation_time = rand() % MAX_WAITING_TIME + 1;
	sleep(consultation_time + 1);
 
	increase_doctor_count(current_doctor_id,waiting_time,consultation_time);

	pthread_mutex_lock(&mtx);
	threads_still_running--;
	pthread_mutex_unlock(&mtx);
}
 
int main(int argc, char* argv[])
{
	pthread_t thr[MAX_CLIENTS];
	int i;
	int actual_clients = 0;
	srand(time(NULL));	
 
	if (pthread_mutex_init(&mtx,NULL))
	{
		perror("Error when creating the mutex");
		return errno;
	}
 
	for (i=0;i<MAX_CLIENTS && current_time < CLIENT_GENERATING_TIME;i++)
	{
		if (pthread_create(&thr[i],NULL,consultation,NULL))
		{ 
			perror("Error when creating a thread");
			return errno;
		}
 
		threads_still_running++;
		actual_clients++;
		time_pass(rand() % MAX_TIME_BETWEEN_CLIENTS + 1); 		// We wait for the next client
	}
 
	while (threads_still_running > 0)
	{
		time_pass(1);
	}
 
	void *result;
	for (i=0;i<actual_clients;i++)
	{
		if (pthread_join(thr[i], &result))
		{
			perror("Error when joining result");
			return errno;
		}
	}
 
	printf("\nAverage waiting time: %d second(s)",total_waiting_time/actual_clients);
	printf("\nEnd of work day\n");
	pthread_mutex_destroy(&mtx);
 
	return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>

#define ERROR(str) { perror(str); exit(-1); }

char *getTimeStr( char *buf )
{
	// time vars
	struct timeval tv_now;
	// get msecs
	gettimeofday( &tv_now, NULL );
	// get specs
	int msecs = tv_now.tv_usec / 1000;
	int secs  = tv_now.tv_sec % 60;
	int mins  = (tv_now.tv_sec / 60) % 60;
	int hrs   = (tv_now.tv_sec / 3600 + 3) % 24; // +3 is gmt
	
	sprintf(buf, "%02d:%02d:%02d:%03d", hrs, mins, secs, msecs );
	return buf;
}


void * thread_proc(void * param)
{
	// print info
	char buf[36];
	printf("Th num %ld. My PID is %d, parents PID is %d. time now %s\n", (long)param, getpid(), getppid(), getTimeStr(buf));
	
	return 0;
}

#define NUM_OF_THREADS 2

int main()
{
	pthread_t pth_arr[NUM_OF_THREADS] = { -1 };
	
	// create threads
	for (int i = 0; i < NUM_OF_THREADS; i++)
	{
		if (pthread_create(pth_arr + i, NULL, thread_proc, (void *)((long)i + 1)) == 0)
			puts("Thread created!");
		else
			perror("Thread could not be created!");
	}
	
	// print info
	char buf[36];
	printf("Th main. My PID is %d, parents PID is %d. time now %s\n", getpid(), getppid(), getTimeStr(buf));
	
	// join threads
	for (int i = 0; i < NUM_OF_THREADS; i++)
	{
		if ((pth_arr[i] != -1) && pthread_join(pth_arr[i], NULL) == 0)
			puts("Thread joined!");
		else
			perror("Thread could not be joined!");
	}

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>

#define ERROR(str) { perror(str); exit(-1); }

#include <string.h>
#include <dirent.h>

#include <limits.h> // here is PATH_MAX

#define FALSE 0
#define TRUE 1

typedef struct _th_rcd 
{
	pthread_t th;
	int valid;
	char path[PATH_MAX];
} th_rcd;

// thread structures arr
int th_num, th_next;
th_rcd *th_arr;

// for recursive directory visiting
char name_file_A_full[PATH_MAX];
char *name_file_A;

// for cph
#define MAX_KEY_LEN 256
char cph_key[MAX_KEY_LEN];
int cph_len;

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
	long th_num = (long)param;
	
	// do algorithm
	// try to open file
	FILE* f  = fopen (th_arr[th_num].path , "rb+");
	if ( !f ) {
		perror("Cant open file to read!");
		return 0;
	}
		
	// cmp em symbol per symbol
	char next; long cnt = 0; 
	
	while ( fread( &next, sizeof(char) , 1, f ) )
	{
		char cph = next ^ cph_key[cnt % cph_len]; // enchipher
		//cph_key ^= next;
		
		// move file ptr back
		fseek( f, -sizeof(char), SEEK_CUR );
		
		if ( !fwrite(&cph, sizeof(char) , 1, f) ) // put back
			perror("Cant put symbol back!");
			
		cnt++; // inc num of looked bytes
		
	}
	
	// print info
	char buf[36];
	printf("Th num %ld. My PID is %d, parents PID is %d. time now %s\nPath: %s. Cph %ld bytes.\n", th_num + 1, getpid(), getppid(), getTimeStr(buf), th_arr[th_num].path, cnt);
	
	// clear myself in array
	th_arr[th_num].valid = FALSE;
	
	return 0;
}

void wait_for_all_threads()
{
	for ( int i = 0; i < th_num; i++ ) // end all threads
	{
		// wait & join every thread
		if (!th_arr[i].valid) // do not need to wait
			continue;
		else if (pthread_join(th_arr[i].th, NULL) == 0)
			puts("Thread joined!");
		else
			perror("Thread could not be joined!");
	}
}

void dir_visit( DIR *dir_A )
{
	struct dirent *dirent_A;  
	
	while ( dirent_A = readdir(dir_A) )
	{
		const char *cur_name = dirent_A->d_name;
		if ( !strcmp(cur_name, ".") || !strcmp(cur_name, "..") )
			continue;
		// get file name
		strcpy(name_file_A, cur_name);
		
		// detect what to do
		if ( dirent_A->d_type & DT_DIR )
		{
			// get dir
			DIR *dir = opendir( name_file_A_full );  
			// try dir
			if (!dir)
				ERROR("Cant open dir!");
				
			// move rel path (name_file_A)  
			int offset = strlen(cur_name);
			name_file_A += offset;
			*(name_file_A++) = '/'; // now str has not \0!!
			
			// visit recursively
			dir_visit( dir );
			// reseth path back
			name_file_A-= offset + 1;
			
			// close dir
			if (closedir(dir))
				ERROR("Error while dir close occured!");
		}
		else if ( dirent_A->d_type & DT_REG )
		{
			// check if can create new thread
			while (th_arr[th_next].valid) 
			{
				th_next = (th_next + 1) % th_num;
				if (th_next == 0)
					usleep(100);
			}
			// prepare data (bc we dont know, what ll start earlier)
			strcpy( th_arr[th_next].path, name_file_A_full );
			th_arr[th_next].valid = TRUE;
			// launch thread
			if (!pthread_create(&(th_arr[th_next].th), NULL, thread_proc, (void *)((long)th_next)) == 0) {
				// if failed
				th_arr[th_next].valid = FALSE;
				perror("Thread could not be created!");
			}
		} 
	}

}

int main( int argc, char *argv[] )
{
	// check if it is enough params passed
	if (argc != 4)
		ERROR("Wrong args! Format: Task_addl.exe [dir] [num of threads runned simultaniosly] [key str]");
	// get numb of procs
	//th_num = 0; 
	if ( !sscanf( argv[2], "%u", &th_num ) || (th_num < 1) )
		ERROR("Wrong args! Format: 2nd param is [num of threads runned simultaniosly]");
	// get key
	strncpy(cph_key, argv[3], MAX_KEY_LEN - 1 );
	cph_len = strlen(cph_key); 
	if (cph_len < 1)
		ERROR("Wrong args! Format: 3rd param is [key str]");
	
	// === open dirs ===
	// get dir A
	DIR *dir_A = opendir( argv[1] );  
	// try dir
	if (!dir_A)
		ERROR("Wrong args! Format: 1st param is [dir]");
	// === alloc place for th ids ===
	th_arr = calloc(th_num, sizeof(th_rcd));
	if (!th_arr)
		ERROR("Cannot allocate array with size [num of threads runned simultaniosly]!");
			
	// === cph ===
	// setup file base paths
	strcpy(name_file_A_full, argv[1] );
	name_file_A = name_file_A_full + strlen(name_file_A_full);
	*(name_file_A++) = '/';
	
	// run for all files in dir recursively
	dir_visit( dir_A );
			
	// wait for all started processess	
	wait_for_all_threads( );
	// free mem
	free( th_arr );
	
	// close dir A
	if (closedir(dir_A))
		ERROR("Error while dir close occured!");

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "fractal.h"

pthread_mutex_t mutex;
int maxthreads = 1;
pthread_t *threads;
struct fractal **frac;
struct fractal best_frac;
int *Status;
int DoEach = 0;
int Keep = 0;
sem_t empty;
sem_t full;
enum { FREE, COMPUTING, ALLOCATED };



void *fractal_compute(void *args) //Consommateur
{
	struct fractal *f;
	int nFrac = -1;
	while(Keep == 1) //Doit savoir quand il n'aura plus rien à calculer
	{
		sem_wait(&full);
		//Recherche d'une nouvelle fractale
		for(int i = 0; i < maxthreads-1; i++)
		{
			if(Status[i] == ALLOCATED) 
			{
				f = frac[i];
				nFrac = i;
				break;
			}
		}
		pthread_mutex_lock(&mutex);
		Status[nFrac] = COMPUTING;
		pthread_mutex_unlock(&mutex);
		
		//Calcul de la fractale
		double average = 0;
		for(int i = 0; i < f->w; i++)
		{
			for(int j = 0; j < f->h; j++)
			{
				fractal_compute_value(f, i, j);
				average += fractal_get_value(f, i, j);
			}
		}
		f->average = average / (f->w * f->h);
		
		pthread_mutex_lock(&mutex);
		
		if(f->average > best_frac.average) best_frac = (*f);
		fractal_free(f);		
		Status[nFrac] = FREE;
				
		pthread_mutex_unlock(&mutex);
		sem_post(&empty);
	}
	printf("Hello fractalou!\n");
}


int main(int argc, char *argv[]) //Producteur
{
	//Initialisations
	char **filename;
	char *filenameOut;
	int nbfiles = 0;
	int readstdin = 0;
	
	//Lecture des arguments
	filename = (char**)malloc(sizeof(char*) * (argc-1));
	for(int i = 0; i < argc-1; i++)
	{
		if(argv[i] == "-d") DoEach = 1;
		if(argv[i] == "--maxthreads" && i+1<argc) maxthreads = atoi(argv[i+1]);
		if(argv[i] == "-") readstdin = 1;
		else { filename[nbfiles] = argv[i]; nbfiles++; }
	}
	filenameOut = argv[argc-1];
	
	//Allocations et initialisations
	threads = (pthread_t*)malloc(sizeof(pthread_t)*maxthreads);
	frac = (struct fractal **)malloc(sizeof(struct fractal*)*maxthreads);
	Status = (int*)calloc(maxthreads, sizeof(int));
	if(threads == NULL || frac == NULL || Status == NULL)
	{
		perror("malloc");
		return EXIT_FAILURE;
	}
	sem_init(&empty, 0, maxthreads); 
	sem_init(&full, 0, 0);
	for(int i = 0; i < maxthreads; i++)
	{
		if(pthread_create(&threads[i], NULL, fractal_compute, NULL) == -1) 
		{
			perror("pthread_create");
			printf("Problem with pthread_create\n");
			return EXIT_FAILURE;
		}
	}
	
	//Production
	char *frac_name;
	int frac_w, frac_h;
	double frac_a, frac_b;
	FILE *actFile;
	int inc_file = 0;
	int nFrac = -1;
	struct fractal *f;
	while(Keep == 1)
	{
		//Creation de la fractale
		if(inc_file == nbfiles)
		{
			Keep = 0;
		}
		else if(actFile == NULL)
		{
			if(actFile = fopen(filename[inc_file],"r")) == NULL)
			{
				perror("fopen");
				printf("Problem with fopen\n");
				return EXIT_FAILURE;
			}
			inc_file++;
		}
		for(int i = 0; i < maxthreads; i++)
		{
			if(Status[i] == FREE) 
			{
				f = frac[i];
				
				nFrac = i;
			}
		}
		sem_wait(&empty);
		
		
		pthread_mutex_lock(&mutex);
		//insert_item();
		pthread_mutex_unlock(&mutex);
		sem_post(&full);
	}
	

	
	for(int i = 0; i < maxthreads; i++)
	{
		if (pthread_join(threads[i], NULL)) 
		{
			perror("pthread_join");
			printf("Problem with pthread_join\n");
			return EXIT_FAILURE;
		}
    }
	
	if(write_bitmap_sdl(&best_frac, "Fractal.bmp") == 0) printf("Done !\n");
	else 
	{
		perror("write_bitmap_sdl");
		printf("Problem with write_bitmap_sdl\n");
		return EXIT_FAILURE;
	}
	
	
	
    return 0;
}

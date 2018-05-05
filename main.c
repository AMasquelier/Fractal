#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "fractal.h"

pthread_mutex_t mutex;
int maxthreads = 2;
pthread_t *threads;
struct fractal **frac;
struct fractal best_frac;
int *Status;
int DoEach = 0;
int Keep = 1;
sem_t empty;
sem_t full;
enum { FREE, COMPUTING, ALLOCATED };



void *fractal_compute(void *args) //Consommateur
{
	struct fractal *f;
	printf("FractCompute\n");
	int nFrac = -1;
	while(Keep == 1) //Doit savoir quand il n'aura plus rien à calculer
	{
		sem_wait(&full);
		//Recherche d'une nouvelle fractale
		
		for(int i = 0; i < maxthreads; i++)
		{
			
			printf("fract %d\n", Status[nFrac]);
			if(Status[i] == ALLOCATED) 
			{
				f = frac[i];
				nFrac = i;
				printf("Frac n° %s\n", f->name);
				break;
			}
		}
		pthread_mutex_lock(&mutex);
		Status[nFrac] = COMPUTING;
		pthread_mutex_unlock(&mutex);
		
		printf("\n Computing...\n");
		
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
		
		char *buf = strcat(f->name,".bmp");
		if(write_bitmap_sdl(&best_frac, buf) == 0) printf("Done !\n");
		else 
		{
			perror("write_bitmap_sdl");
			printf("Problem with write_bitmap_sdl\n");
			return EXIT_FAILURE;
		}
		
		
		pthread_mutex_lock(&mutex);
		
		if(f->average > best_frac.average) best_frac = (*f);
		fractal_free(f);		
		Status[nFrac] = FREE;
				
		pthread_mutex_unlock(&mutex);
		sem_post(&empty);
	}
	printf("Quit fract\n");
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
	
	for(int i = 1; i < argc-1; i++)
	{
		if(strcmp(argv[i],"-d") == 0) DoEach = 1;
		else if(strcmp(argv[i],"--maxthreads") == 0 && i+1<argc) maxthreads = atoi(argv[i+1]);
		else if(strcmp(argv[i],"-") == 0) readstdin = 1;
		else { filename[nbfiles] = argv[i]; nbfiles++; printf("%s\n", argv[i]);}
	}
	filenameOut = argv[argc-1];
	printf("\n");
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
	char frac_name[64];
	char *trash;
	int frac_w, frac_h;
	float frac_a, frac_b;
	FILE *actFile = NULL;
	int inc_file = 0;
	int nFrac = -1;
	struct fractal *f;
	while(Keep == 1)
	{
		//Creation de la fractale
		for(int i = 0; i < maxthreads; i++)
		{
			if(Status[i] == FREE) 
			{
				f = frac[i];
				nFrac = i;
			}
		}
			//Lecture des fichiers
		if(inc_file > nbfiles-1)
		{
			printf("%d\n", 2);
			if(readstdin == 0) Keep = 0;
			else
			{
				//Lecture de l'entrée standard
				readstdin = 0;
			}
		}
		else if(actFile == NULL)
		{
			printf("%d / %d \n", inc_file, nbfiles);
			actFile = fopen(filename[inc_file],"r");
			if(actFile == NULL)
			{
				perror("fopen");
				printf("Problem with fopen %s\n", filename[inc_file]);
				return EXIT_FAILURE;
			}
 			
			inc_file++;
		}
		
		if(actFile != NULL)
		{
			if(fscanf(actFile, "%s %d %d %f %f\n", frac_name, &frac_w, &frac_h, &frac_a, &frac_b) >= 0)
			{
				printf("%s %d %d %f %f\n", frac_name, frac_w, frac_h, frac_a, frac_b);
			}
			else fclose(actFile);
			
		}
			printf("Waiting...\n");
			sem_wait(&empty);
		
			pthread_mutex_lock(&mutex);
			frac[nFrac] = fractal_new(frac_name, frac_w, frac_h, frac_a, frac_b);
			Status[nFrac] = ALLOCATED;
			
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
	
	if(write_bitmap_sdl(&best_frac, strcat(filenameOut,".bmp")) == 0) printf("Done !\n");
	else 
	{
		perror("write_bitmap_sdl");
		printf("Problem with write_bitmap_sdl\n");
		return EXIT_FAILURE;
	}
	
	
	
    return 0;
}

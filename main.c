#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "fractal.h"

pthread_mutex_t mutex;
int maxthreads = 4;
struct fractal **frac;
struct fractal best_frac;
int *Lock;
int DoEach = 0;
int Keep = 0;
sem_t empty;
sem_t full;



void fractal_compute(void) //Consommateur
{
	struct fractal *f;
	while(Keep == 1) //Doit savoir quand il n'aura plus rien à calculer
	{
		sem_wait(&full);
		
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
		
		if(f->average > best_frac.average) best_frac = *f;
		fractal_free(f);		
				
		pthread_mutex_unlock(&mutex);
		sem_post(&empty);
	}
}


int main(int argc, char *argv[]) //Producteur
{
	//Initialisations
	for(int i = 0; i < argc; i++)
	{
		if(argv[i] == "-d") DoEach = 1;
		if(argv[i] == "--maxthreads" && i+1<argc) maxthreads = atoi(argv[i]);
	}
	Lock = malloc(sizeof(int)*(maxthreads-1));
	sem_init(&empty, 0, maxthreads-1); 
	sem_init(&full, 0, 0);
	
	//Production
	while(Keep == 1)
	{
		
	}
	
	
	Keep = 0;
	
	if(write_bitmap_sdl(&best_frac, "Fractal.bmp") == 0) printf("Done !\n");
	else printf("Error !\n");
	
    return 0;
}

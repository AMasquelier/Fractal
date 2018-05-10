#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include "fractal.h"

pthread_mutex_t mutex;
int maxthreads = 1;
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
	printf("   New thread\n");
	int nFrac = -1;
	while(Keep == 1) //Doit savoir quand il n'aura plus rien à calculer
	{
		sem_wait(&full);
		//Recherche d'une nouvelle fractale
		if(Keep == 0) break;
		
		pthread_mutex_lock(&mutex);
		for(int i = 0; i < maxthreads; i++)
		{
			if(Status[i] == ALLOCATED) 
			{
				f = frac[i];
				nFrac = i;
				break;
			}
		}
		Status[nFrac] = COMPUTING;
		pthread_mutex_unlock(&mutex);
		
		printf("   Computing %s...\n", f->name);
		
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
		
		char buf[64];
		strcpy(buf, f->name);
		strcat(buf,".bmp");
		if(DoEach == 1 && write_bitmap_sdl(frac[nFrac], buf) == 0) printf("   %s done !\n", buf);
		else if(DoEach == 1)
		{
			perror("write_bitmap_sdl");
			printf("Problem with write_bitmap_sdl\n");
		}
		
		
		pthread_mutex_lock(&mutex);
		
		if(f->average > best_frac.average) best_frac = (*f);
		fractal_free(f);		
		Status[nFrac] = FREE;
				
		pthread_mutex_unlock(&mutex);
		sem_post(&empty);
	}
	printf("   Closing thread\n");
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
		else if(strcmp(argv[i],"--maxthreads") == 0 && i+1<argc) {maxthreads = atoi(argv[i+1]); i++; }
		else if(strcmp(argv[i],"-") == 0) readstdin = 1;
		else { filename[nbfiles] = argv[i]; nbfiles++;}
	}
	if(maxthreads < 1)
	{
		printf("Invalid number of threads! ->  Set on default number of threads : 1\n");
		maxthreads = 1;
	}
	if(nbfiles + readstdin > 0) filenameOut = argv[argc-1];
	else
	{
		printf("Out file name not found -> Set on Bitmap.bmp");
		filename[nbfiles] = argv[argc-1]; nbfiles++;
		filenameOut = "Bitmap.bmp";
	}
	printf("\n");
	//Allocations et initialisations
	threads = (pthread_t*)malloc(sizeof(pthread_t)*maxthreads);
	frac = (struct fractal **)malloc(sizeof(struct fractal*)*maxthreads);
	Status = (int*)calloc(maxthreads, sizeof(int));
	if(threads == NULL || frac == NULL || Status == NULL)
	{
		printf("Allocation problem\n");
		perror("malloc");
		return -1;
	}
	sem_init(&empty, 0, maxthreads); 
	sem_init(&full, 0, 0);
	for(int i = 0; i < maxthreads; i++)
	{
		if(pthread_create(&threads[i], NULL, fractal_compute, NULL) == -1) 
		{
			perror("pthread_create");
			printf("Problem with pthread_create\n");
			return -1;
		}
	}
	
	//Production
	char frac_name[64];
	char trash;
	int frac_w, frac_h;
	float frac_a, frac_b;
	FILE *actFile = NULL;
	int inc_file = 0;
	int nFrac = -1;
	struct fractal *f;
	char line[1024];
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
		FileRead:
		
		if(inc_file > nbfiles-1)
		{
			if(readstdin == 0) Keep = 0;
			else
			{
				char c;
				//Lecture de l'entrée standard
				Readstdin:
				printf("Type + to make a fractal and - to quit\n");
				while(scanf("%c", &c) != 1);
				
				if(c == '+')
				{
					printf("Enter a fractal with the format :\n   <name> <w> <h> <a> <b>\n");
				
					if(scanf("%s %d %d %f %f", frac_name, &frac_w, &frac_h, &frac_a, &frac_b)==5)
					{
						if(frac_w <= 0 || frac_h <= 0 || frac_a < -1 || frac_a > 1 || frac_b < -1 || frac_b > 1) 
						{
							printf("Bad format !\n");
						}
					}
					// Vide le buffer de l'entrée standard
					int gc = 0;
					while (gc != '\n' && gc != EOF)
					{
						gc = getchar();
					}
				}
				else if (c == '-') readstdin = 0;
				else goto Readstdin;
			}
		}
		else if(actFile == NULL)
		{
			printf("File %d / %d \n", inc_file+1, nbfiles);
			actFile = fopen(filename[inc_file],"r");
			if(actFile == NULL)
			{
				perror("fopen");
				printf("Problem with fopen %s\n", filename[inc_file]);
				return -1;
			}
 			
		}
		
		if(actFile != NULL && Keep == 1) 
		{
			
			char *token;
			Read:
			fgets(line, 1024, actFile);
			token = strtok(line, " ");
			if(strcmp(token,"#") != 0)
			{
				char buf[64];
				char *endptr;
				strcpy(frac_name, token);
				
				token = strtok(NULL, " ");
				if(token == NULL) { printf("Missing value\n"); goto CloseFile;}	
				strcpy(buf, token);
				frac_w = strtol(buf, &endptr, 10);
			
				token = strtok(NULL, " ");
				if(token == NULL) { printf("Missing value\n"); goto CloseFile;}
				strcpy(buf, token);
				frac_h = strtol(buf, &endptr, 10);
				
				token = strtok(NULL, " ");
				if(token == NULL) { printf("Missing value\n"); goto CloseFile;}
				strcpy(buf, token);
				frac_a = strtod(buf, &endptr);
				
				token = strtok(NULL, " ");
				if(token == NULL) { printf("Missing value\n"); goto CloseFile;}
				strcpy(buf, token);
				frac_b = strtod(buf, &endptr);
				if(frac_w <= 0 || frac_h <= 0 || frac_a < -1 || frac_a > 1 || frac_b < -1 || frac_b > 1) 
				{
					printf("Bad format !, %s\n", frac_name);
					
					goto CloseFile;
				}
			}
			else if(strcmp(token,"#") == 0)
			{				
				goto Read;
			}
			else
			{
				CloseFile:
				printf("Closing file\n");
				inc_file++;
				fclose(actFile);
				actFile = NULL;
				goto FileRead;
			}
			
		}
		if(Keep == 1)
		{
			printf(" Waiting...\n");
			sem_wait(&empty);
		
			pthread_mutex_lock(&mutex);
			frac[nFrac] = fractal_new(frac_name, frac_w, frac_h, frac_a, frac_b);
			Status[nFrac] = ALLOCATED;
			
			pthread_mutex_unlock(&mutex);
			sem_post(&full);
		}
		
	}
	End:
	printf("Waiting for threads...\n");
	
	
	for(int i = 0; i < maxthreads; i++) sem_post(&full); //Pour sortir les threads de calcul de leur attente
	for(int i = 0; i < maxthreads; i++)
	{
		if (pthread_join(threads[i], NULL)) 
		{
			perror("pthread_join");
			printf("Problem with pthread_join\n");
			return -1;
		}
	}
	
	if(best_frac.w != 0 && best_frac.h != 0)
	{
		if(write_bitmap_sdl(&best_frac, filenameOut) == 0) printf("%s done !\n", filenameOut);
		else 
		{
			perror("write_bitmap_sdl");
			printf("Problem with write_bitmap_sdl\n");
			return -1;
		}
	}
	else
	{
		printf("No fractal to save !\n");
	}
	free(threads);
	free(frac);
	free(filename);
	
    return 0;
}

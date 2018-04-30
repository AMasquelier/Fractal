#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fractal.h"


int maxthreads = 4;
struct fractal **frac;
struct fractal best_frac;
int DoEach = 0;


void fractal_compute(struct fractal *f) //Consommateur
{
		
	
	for(int i = 0; i < f->w; i++)
	{
		for(int j = 0; j < f->h; j++)
		{
			fractal_compute_value(f, i, j);
		}
	}
}


int main(int argc, char *argv[]) //Producteur
{
	for(int i = 0; i < argc; i++)
	{
		if(argv[i] == "-d") DoEach = 1;
		if(argv[i] == "--maxthreads" && i+1<argc) maxthreads = atoi(argv[i]);
	}
		
    struct fractal *f;
	f = fractal_new("fractal", 1080, 1080, 0.285, 0.01);
	
	
	if(write_bitmap_sdl(f, "Fractal.bmp") == 0) printf("Done !\n");
	else printf("Error !\n");
	
    return 0;
}

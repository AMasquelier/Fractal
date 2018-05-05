#include <stdlib.h>
#include "fractal.h"

struct fractal *fractal_new(const char *name, int width, int height, double a, double b)
{
  	struct fractal *f = malloc(sizeof(struct fractal));
	strcpy(f->name, name);
	f->w = width;
	f->h = height;
	f->a = a;
	f->b = b;
	f->value = malloc(sizeof(int)*width*height);
	
	f->average = 0;
    return f;
}

void fractal_free(struct fractal *f)
{
    if(f != NULL) free(f);
}

const char *fractal_get_name(const struct fractal *f)
{
    return f->name;
}

int fractal_get_value(const struct fractal *f, int x, int y)
{
    return f->value[x + y * f->w];
}

void fractal_set_value(struct fractal *f, int x, int y, int val)
{
    f->value[x + y * f->w] = val;
}

int fractal_get_width(const struct fractal *f)
{
    return f->w;
}

int fractal_get_height(const struct fractal *f)
{
    return f->h;
}

double fractal_get_a(const struct fractal *f)
{
    return f->a;
}

double fractal_get_b(const struct fractal *f)
{
    return f->b;
}

#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <SDL.h>
#include "fractal.h"

#define MAX_ITER 4096
#define itoc(x) ((0x00ffffff/MAX_ITER)*(x))

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RMASK 0xff000000
#define GMASK 0x00ff0000
#define BMASK 0x0000ff00
#define AMASK 0x000000ff
#else
#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#endif

int write_bitmap_sdl(const struct fractal *f, const char *fname)
{
    SDL_Surface *back;
    SDL_Rect pix;
    int w, h, i, j, col;
    uint32_t pcol, rcol, gcol, bcol;
    w = fractal_get_width(f);
    h = fractal_get_height(f);

    back = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, RMASK, GMASK, BMASK, AMASK);
    if (!back)
        return -1;

    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            col = itoc(fractal_get_value(f, i, j));
            rcol = col & 0xff;
            gcol = (col >> 8) & 0xff;
            bcol = (col >> 16) & 0xff;
            pcol = SDL_MapRGB(back->format, rcol, gcol, bcol);

            pix.w = pix.h = 1;
            pix.x = i;
            pix.y = j;
            SDL_FillRect(back, &pix, pcol);
        }
    }

    if (SDL_SaveBMP(back, fname) < 0)
        return -1;

    SDL_FreeSurface(back);

    return 0;
}

static int iter_julia(double zx, double zy, double a, double b, int it)
{
    /* prevent infinite loop */
    if (it > MAX_ITER)
        return 0;

    /* prevent leaving Julia set
     * if distance to origin >= 2
     */
    if (zx*zx + zy*zy >= 4.0)
        return it;

    /* compute next iteration
     * f(z) = z^2 + c
     * z = x + yi
     * c = a + bi
     */
    return iter_julia(zx*zx - zy*zy + a, 2*zx*zy + b, a, b, it+1);
}

int fractal_compute_value(struct fractal *f, int x, int y)
{
    double zx, zy;
    double a, b;
    int val;
    int w, h;

    a = fractal_get_a(f);
    b = fractal_get_b(f);

    w = fractal_get_width(f);
    h = fractal_get_height(f);

    zx = ((double)x / (double)w) * 3.0 - 1.5;
    zy = ((double)y / (double)h) * 3.0 - 1.5;

    val = iter_julia(zx, zy, a, b, 0);
    fractal_set_value(f, x, y, val);

    return val;
}

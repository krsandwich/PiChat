#include "gl.h"
#include "font.h"
#include "malloc.h"
#include "strings.h"

#include "printf.h"

#define WIDTH gl_get_width()
#define HEIGHT gl_get_height()
#define CHARWIDTH gl_get_char_width()
#define CHARHEIGHT gl_get_char_height()
#define DEPTH 4

static unsigned int pitch;

static int min(int a, int b){
    return (a<b)? a : b;
}

static int max(int a, int b){
    return (a>b)? a : b;
}

void gl_init(unsigned int width, unsigned int height, unsigned int mode)
{
    fb_init(width, height, DEPTH, mode);
    pitch = fb_get_pitch()/DEPTH;
}

void gl_swap_buffer(void)
{
    fb_swap_buffer();
}

unsigned int gl_get_width(void) 
{
    return fb_get_width();
}

unsigned int gl_get_height(void)
{
    return fb_get_height();
}

color gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    return (0xFF<< 24) + (r<<16) + (g<<8) + b;
}

void gl_clear(color c)
{
    for(int x=0; x<WIDTH; x++){
        for(int y=0; y<HEIGHT; y++){
            //using ints only works with DEPTH=4 but only need to work with that
            unsigned (*im)[pitch] = (unsigned (*)[pitch])fb_get_draw_buffer();
            im[y][x] = c;
        }
    }
}

void gl_draw_pixel(int x, int y, color c)
{
    if(x < WIDTH && x >= 0 && y < HEIGHT && y >= 0){
        //using ints only works with DEPTH=4 but only need to work with that
        unsigned (*im)[pitch] = (unsigned (*)[pitch])fb_get_draw_buffer();
        im[y][x] = c;
    }
}

color gl_read_pixel(int x, int y)
{
    //using ints only works with DEPTH=4 but only need to work with that
    unsigned (*im)[pitch] = (unsigned (*)[pitch])fb_get_draw_buffer();
    return im[y][x];
}

void gl_draw_rect(int x, int y, int w, int h, color c)
{
    int xFinal = min(x + w, WIDTH);
    int yFinal = min(y + h, HEIGHT);
    int xCoordinate = max(x, 0);
    int yCoordinate = max(y, 0);

    for(int i=xCoordinate; i<xFinal; i++){
        for(int j=yCoordinate; j<yFinal; j++){
            unsigned (*im)[pitch] = (unsigned (*)[pitch])fb_get_draw_buffer();
            im[j][i] = c;
        }
    }
}

void gl_draw_char(int x, int y, int ch, color c)
{
    int index = 0;

    int size = font_get_size();
    unsigned char *buf = (unsigned char*) malloc(size);

    font_get_char((unsigned char) ch, buf, size);


    int xFinal = min(x + CHARWIDTH, WIDTH);
    int yFinal = min(y + CHARHEIGHT, HEIGHT);
    int xCoordinate = max(x, 0);
    int yCoordinate = max(y, 0);

    for(int j=yCoordinate; j<yFinal; j++){
        for(int i=xCoordinate; i<xFinal; i++){
            if(buf[index] != 0){
                unsigned (*im)[pitch] = (unsigned (*)[pitch])fb_get_draw_buffer();
                im[j][i] = c;
            }
            index++;
        }
    }
}

void gl_draw_string(int x, int y, char* str, color c)
{
    int length = strlen(str);
    int charwidth = CHARWIDTH;
    for(int i=0; i<length; i++){
        gl_draw_char(x+charwidth * i, y, str[i], c);
    }
}

unsigned int gl_get_char_height(void)
{
    return font_get_height();
}

unsigned int gl_get_char_width(void)
{
    return font_get_width();
}

// NOTE: code inspired by pseudo code at https://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm
static void plot(int x, int y, color c, float brightness){
    color old = gl_read_pixel(x, y);
    
    int oldBlue = old % 0x100;
    int oldGreen = (old >> 8) % 0x100;
    int oldRed = (old >> 16) % 0x100;

    int cBlue = c % 0x100;
    int cGreen = (c >> 8) % 0x100;
    int cRed = (c >> 16) % 0x100;

    int diffBlue = cBlue - oldBlue;
    int diffGreen = cGreen - oldGreen;
    int diffRed = cRed - oldRed;

    int newBlue = oldBlue + (int)(brightness * diffBlue);
    int newGreen = oldGreen + (int)(brightness * diffGreen);
    int newRed = oldRed + (int)(brightness * diffRed);

    color new = gl_color(newRed, newGreen, newBlue);

    gl_draw_pixel(x,y,new);
}

static int abs(int x){
    return (x > 0)? x : -x;
}

static int floor(float x){
    return (int) x; // only works if x>0, but that's all we're dealing with
}

static float fpart(float x){
    return x - floor(x);
}

static float rfpart(float x){
    return 1 - fpart(x);
}

static void swap(int *x, int *y){
    int temp = *x;
    *x = *y;
    *y = temp;
}

void gl_draw_line(int x1, int y1, int x2, int y2, color c){
    int steep = abs(y1-y2) > abs(x1-x2);

    if(steep){
        swap(&x1, &y1);
        swap(&x2, &y2);
    }

    if(x1 > x2){
        swap(&x1, &x2);
        swap(&y1, &y2);
    }

    float dx = x2 - x1;
    float dy = y2 - y1;
    float grad = dy/dx;

    int xpxl1 = x1;
    float intery = y1;
    int xpxl2 = x2;

    if(steep){
        for(int x = xpxl1; x<= xpxl2; x++){
            plot(floor(intery), x, c, rfpart(intery));
            plot(floor(intery) + 1, x, c, fpart(intery));
            intery += grad;
        }
    }
    else{
        for(int x = xpxl1; x<= xpxl2; x++){
            plot(x, floor(intery), c, rfpart(intery));
            plot(x, floor(intery) + 1, c, fpart(intery));
            intery += grad;
        }
    }

}

// calculates whether a point is in the triangle using barycentric coordinates
// https://stackoverflow.com/questions/13300904/determine-whether-point-lies-inside-triangle
static int pointInTriangle(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3){
    int denom = (y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3);

    if(denom == 0){
        return 0;
    }

    float a = ((y2 - y3)*(x - x3) + (x3 - x2)*(y- y3)) / (float)denom;
    float b = ((y3 - y1)*(x - x3) + (x1 - x3)*(y - y3)) / (float)denom;
    float g = 1.0f - a - b;

    return a > 0 && b > 0 && g > 0;
}

void gl_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, color c){
    gl_draw_line(x1, y1, x2, y2, c);
    gl_draw_line(x1, y1, x3, y3, c);
    gl_draw_line(x2, y2, x3, y3, c);

    int xMin = min(x1, min(x2,x3));
    int xMax = max(x1, max(x2,x3));
    int yMin = min(y1, min(y2,y3));
    int yMax = max(y1, max(y2,y3));

    for(int i=xMin; i<=xMax; i++){
        for(int j=yMin; j<=yMax; j++){
            if(pointInTriangle(i,j,x1,y1,x2,y2,x3,y3)){
                gl_draw_pixel(i,j,c);
            }
        }
    }
}

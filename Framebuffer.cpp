#include "Framebuffer.h"
#include <memory.h>

void Framebuffer::recreate(int width, int height){
    this->width = width;
    this->height = height;

    patternbuffer = new patternbuffer_t[width * height];
    depthbuffer = new depthbuffer_t[width * height];
    fillbuffer = new fillbuffer_t[width * height];
    clear(0);
}

void Framebuffer::clear(uint8_t clearPattern){
    memset(patternbuffer, clearPattern, sizeof(patternbuffer_t) * width * height);
    memset(depthbuffer, MAX_DEPTH_VALUE, sizeof(depthbuffer_t) * width * height);
    memset(fillbuffer, 0, sizeof(fillbuffer_t) * width * height);
}

/**
 * Update the braille pattern stored in the frame buffer based on the inputted pattern, depth and fill. 
 * param x: the column index of the braille pattern. 
 * param y: the row index of the braille pattern. 
 * param pattern: the dithering / shading pattern to apply. 
 * param depth: the distance from the camera. 
 * param fill: the silhouette / area of dots in which the triangle occupies. 
 */
void Framebuffer::setPattern(int x, int y, patternbuffer_t pattern, depthbuffer_t depth, fillbuffer_t fill){
	int index = (y * width) + x;
	
	// check if the new pattern is in front of the old pattern
	if(depth < depthbuffer[index]) {
		// update the depth
		depthbuffer[index] = depth;
		// the updated pattern is the combination of two patterns:
		// the fill of the new pattern, and its anti-fill paired with the old pattern
		patternbuffer[index] = (fill & pattern) | (~fill & patternbuffer[index]);
	} 
	// else if the pattern in front is not entirely full
	else if(fillbuffer[index] != 0xFF) {
		// add the the new pattern in the area where the old pattern is not filled
		patternbuffer[index] |= (~fillbuffer[index] & fill & pattern);
	}

	// combine the old and new fill area
	fillbuffer[index] |= fill;
}

patternbuffer_t Framebuffer::getPattern(int x, int y){
    int index = (y * width) + x;
    return patternbuffer[index];
}

depthbuffer_t Framebuffer::getDepth(int x, int y){
    int index = (y * width) + x;
    return depthbuffer[index];
}

Framebuffer::Framebuffer(int width, int height)
    :width(width), height(height)
{
    recreate(width, height);
}

Framebuffer::~Framebuffer(){
    delete[] patternbuffer;
    delete[] depthbuffer;
    delete[] fillbuffer;
}

void Framebuffer::print(){
	wchar_t wchar;
	int i, j;
	int index;
	for(i = 0; i < width; i++){
		for(j = 0; j < height; j++){
			index = (width * j) + i;
			if(patternbuffer[index]) {
				// calculate the braille pattern value
				wchar = BRAILLE_VAL + patternbuffer[index];
				mvaddnwstr(j, i, &wchar, 1);
			}
		}
	}
}

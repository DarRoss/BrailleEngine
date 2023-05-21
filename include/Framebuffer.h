#ifndef INCLUDE_FRAMEBUFFER_H
#define INCLUDE_FRAMEBUFFER_H

#include "defs.h"

typedef uint16_t depthbuffer_t;
typedef uint8_t patternbuffer_t;
typedef uint8_t fillbuffer_t;

#define MAX_DEPTH_VALUE 0xFFFF

class Framebuffer{
    public:
        Framebuffer(int width, int height);
        ~Framebuffer();

        void recreate(int width, int height);
        void clear(uint8_t clearPattern);

        void setPattern(int x, int y, patternbuffer_t pattern, depthbuffer_t depth, fillbuffer_t fill);
        
        patternbuffer_t getPattern(int x, int y);
        depthbuffer_t getDepth(int x, int y);

        inline int getWidth() const {return width;}
        inline int getHeight() const {return height;}

        void print();

    private:
        int width, height;

        depthbuffer_t* depthbuffer;
        patternbuffer_t* patternbuffer;
        fillbuffer_t* fillbuffer;
};

#endif

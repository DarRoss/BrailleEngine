#ifndef INCLUDE_RASTERIZER_H
#define INCLUDE_RASTERIZER_H

#include "Framebuffer.h"
#include "VertexArrayObject.h"
#include "ShaderProgram.h"
#include <thread>

#define MAX_BRIGHT_VALUE 8

class Vector2;
class Vector3;
class Vector4;
class Matrix44;

class Rasterizer{
    public:
        Rasterizer(int width, int height);
        virtual ~Rasterizer();

        void rasterizeTriangle(const Vector4& v1, const Vector4& v2, const Vector4& v3, ShaderProgram& shader, DataList** passAttributes);

        void presentFrame();

        inline void clearFrame(){
            rFrame->clear(0);
        }

        inline void swapBuffers(){
            currentBuffer ^= 1;
            rFrame = frameBuffers[currentBuffer ^ 1];
            pFrame = frameBuffers[currentBuffer];
        }

        inline void setRenderCB(bool(*rendercb)()){
            renderCallback = rendercb;
        }

    private:
	/**
	 * Dot values are used for calculating the unicode value of a given braille pattern. 
	 * The pattern value is obtained by adding the respective dot values to 0x2800. 
	 * Dot values: 
	 * [ +1][  +8]
	 * [ +2][ +16]
	 * [ +4][ +32]
	 * [+64][+128]
	 */
	const uint8_t DOT_VALUES[4][2] = {{1, 8}, {2, 16}, {4, 32}, {64, 128}};
	// arbitrary brightness patterns to be rendered, given a brightness from 0 to 8 inclusive
	const uint8_t SHADING_PATTERNS[9] = {0x00, 0x20, 0x21, 0x2A, 0x6A, 0x6B, 0x7D, 0xFD, 0xFF};
        bool(*renderCallback)();
        Framebuffer* pFrame;
        Framebuffer* rFrame;
        Framebuffer* frameBuffers[2];
        int currentBuffer;
        void initializeFramebuffer(int width, int height);
        
        std::thread renderThread;
	void updatePattern(int row, int col, const Vector4& vv1, const Vector4& vv2, const Vector4& vv3, const Vector2& v1, const Vector2& v2, const Vector2& v3, ShaderProgram& shader, DataList** passAttributes, int rawSize);

};

class RenderContext {
    public:
        RenderContext(int fbWidth, int fbHeight) :
            r(fbWidth, fbHeight)
        {}

        void renderIndexedTriangles(ShaderProgram& shader, VertexArrayObject& vao);

        Rasterizer* getRasterizer() {
            return &r;
        }

    private:
        Rasterizer r;
};

#endif

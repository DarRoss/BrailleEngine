#include "Rasterizer.h"
#include "defs.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix44.h"

#include <math.h>
#include <atomic>

#define PASS_ATTRIBUTE_V1 0
#define PASS_ATTRIBUTE_V2 1
#define PASS_ATTRIBUTE_V3 2
#define PASS_ATTRIBUTE_OUT 3

std::atomic<bool> renderStart, renderFinished, programRunning;

void renderThreadF(bool(**renderCB)()){
    while(programRunning){
        if(renderStart){
            renderStart = false;
            renderFinished = false;

            if(*renderCB != nullptr){
                (*renderCB)();
            }

            renderFinished = true;
        }
    }
}

void getBarycentricCoordinates(int ptx, int pty, const Vector2& v1, const Vector2& v2, const Vector2& v3, Vector3& coords) {
    coords.x = ((v2.y - v3.y) * (ptx - v3.x) +
            (v3.x - v2.x) * (pty - v3.y)) /
        ((v2.y - v3.y) * (v1.x - v3.x) +
         (v3.x - v2.x) * (v1.y - v3.y));

    coords.y = ((v3.y - v1.y) * (ptx - v3.x) +
            (v1.x - v3.x) * (pty - v3.y)) /
        ((v2.y - v3.y) * (v1.x - v3.x) +
         (v3.x - v2.x) * (v1.y - v3.y));

    coords.z = 1.0f - coords.x - coords.y;
}

static inline bool isPointInTriangle(const Vector3& barycentricCoords)
{
    int one = (barycentricCoords.x < -0.001);
    int two = (barycentricCoords.y < -0.001);
    int three = (barycentricCoords.z < -0.001);

    //is the point in the triangle
    return ((one == two) && (two == three));
}

void Rasterizer::presentFrame(){
    renderStart = true;
    pFrame->print();

    while(!renderFinished);
}

void Rasterizer::initializeFramebuffer(int width, int height){
    frameBuffers[0] = new Framebuffer(width, height);
    frameBuffers[1] = new Framebuffer(width, height);
    swapBuffers();
}

Rasterizer::Rasterizer(int width, int height)
    :renderCallback(nullptr),
    pFrame(nullptr),
    rFrame(nullptr),
    currentBuffer(0)
{
    programRunning = true;
    renderStart = false;
    renderFinished = false;

    initializeFramebuffer(width, height);
    renderThread = std::thread(renderThreadF, &renderCallback);
}

Rasterizer::~Rasterizer(){
    delete frameBuffers[0];
    delete frameBuffers[1];
}

/**
 * Given the vertices of a triangle, update all framebuffer braille patterns that touch this triangle. 
 * param vv1, vv2, vv3: 	vertices of the triangle in 3d space after division by z. 
 * param shader: 		the class containing shader functions. 
 * param passAttributes: 	the list of vertices of the triangle in 3d space. 
 */
void Rasterizer::rasterizeTriangle(const Vector4& vv1, const Vector4& vv2, const Vector4& vv3, ShaderProgram& shader, DataList** passAttributes){
	Framebuffer* fb = rFrame;
	int rawSize = 0;
	int i, j;
	int h_width = fb->getWidth() / 2 * BRAILLE_W, h_height = fb->getHeight() / 2 * BRAILLE_H;
	int minx, maxx;
	int miny, maxy;
	// calculate the vertices of the triangle after 2d projection
	Vector2 v1 = Vector2(vv1.x * h_width + h_width, -vv1.y * h_height + h_height);
	Vector2 v2 = Vector2(vv2.x * h_width + h_width, -vv2.y * h_height + h_height);
	Vector2 v3 = Vector2(vv3.x * h_width + h_width, -vv3.y * h_height + h_height);
	// calculate the 2d edge vectors of the triangle
	Vector2 e1 = v2 - v1;
	Vector2 e2 = v3 - v2;
	// check if triangle is facing camera
	if(Vector2::cross(e1, e2) < 0) {
		//allocate pass space
		for(i = 0; i < passAttributes[PASS_ATTRIBUTE_V1]->getTotalCount(); i++) {
			passAttributes[PASS_ATTRIBUTE_OUT]->bind(nullptr, passAttributes[PASS_ATTRIBUTE_V1]->getLocationSize(i));
			rawSize += passAttributes[PASS_ATTRIBUTE_V1]->getLocationSize(i);
		}
		// calculate bounding box of triangle, measured in terminal chars
		minx = MAX(0, MIN(v1.x, MIN(v2.x, v3.x)) / BRAILLE_W);
		miny = MAX(0, MIN(v1.y, MIN(v2.y, v3.y)) / BRAILLE_H);
		
		maxx = MIN(fb->getWidth(), MAX(v1.x, MAX(v2.x, v3.x)) / BRAILLE_W + 1);
		maxy = MIN(fb->getHeight(), MAX(v1.y, MAX(v2.y, v3.y)) / BRAILLE_H + 1);
		// traverse rows and cols in bounding box
		for(j = miny; j < maxy; j++){
			for(i = minx; i < maxx; i++){
				updatePattern(j, i, vv1, vv2, vv3, v1, v2, v3, shader, passAttributes, rawSize);
			}
		}
	}
}

/**
 * For the braille character at the given row and column, check if each dot is inside a given triangle and update the pattern. 
 * param row: 			the vertical index of the braille character. 
 * param col: 			the horizontal index of the braille character. 
 * param vv1, vv2, vv3: 	vertices of the triangle in 3d space after division by z. 
 * param v1, v2, v3: 		vertices of the triangle on the 2d plane after projection. 
 * param shader: 		the class containing shader functions. 
 * param passAttributes: 	the list of vertices of the triangle in 3d space. 
 * param rawSize: 		the pass space size. 
 */
void Rasterizer::updatePattern(int row, int col, const Vector4& vv1, const Vector4& vv2, const Vector4& vv3, const Vector2& v1, const Vector2& v2, const Vector2& v3, ShaderProgram& shader, DataList** passAttributes, int rawSize) {
	int offsetX, offsetY;
	int i;
	int dotX, dotY;
	float denominator;
	float value1, value2, value3;
	float interpolatedValue;
	fillbuffer_t fill = 0;
	patternbuffer_t pattern = 0;
	depthbuffer_t depth = 0;
	Vector3 barycentric, pcBarycentric;
	Vector4 fragmentOut;
	Framebuffer* fb = rFrame;

	// traverse columns of the pattern
	for(offsetX = 0; offsetX < BRAILLE_W; ++offsetX) {
		// traverse rows of the pattern
		for(offsetY = 0; offsetY < BRAILLE_H; ++offsetY) {
			dotX = BRAILLE_W * col + offsetX;
			dotY = BRAILLE_H * row + offsetY;
			getBarycentricCoordinates(dotX, dotY, v1, v2, v3, barycentric);
			// proceed if this dot is inside the triangle
			if(isPointInTriangle(barycentric)){
				// check if the previous dots were not in the triangle
				// (code in this statement will only be executed once if at least one dot is inside the triangle)
				if(!fill) {
					//calculate corrected barycentric
					denominator = (barycentric.x / vv1.w) + (barycentric.y / vv2.w) + (barycentric.z / vv3.w);
					pcBarycentric.x = (barycentric.x / vv1.z) / denominator;
					pcBarycentric.y = (barycentric.y / vv2.z) / denominator;
					pcBarycentric.z = (barycentric.z / vv3.z) / denominator;
					//loop through each value in the pass buffer and interpolate the value
					//inverse Z allows for translation from AFFINE interpolation
					//allowing projection space interpolation
					for(i = 0; i < rawSize; i++) {
						value1 = passAttributes[PASS_ATTRIBUTE_V1]->getRawValue(i);
						value2 = passAttributes[PASS_ATTRIBUTE_V2]->getRawValue(i);
						value3 = passAttributes[PASS_ATTRIBUTE_V3]->getRawValue(i);
						
						//interpolate between the three values using barycentric interpolation
						interpolatedValue = (value1 * pcBarycentric.x) + (value2 * pcBarycentric.y) + (value3 * pcBarycentric.z);
						passAttributes[PASS_ATTRIBUTE_OUT]->setRawValue(i, interpolatedValue);
					}
					// calculate the depth of this fragment and use it to render to the framebuffer
					depth = MAX_DEPTH_VALUE * ((vv1.z / vv1.w) * barycentric.x + (vv2.z / vv2.w) * barycentric.y + (vv3.z / vv3.w) * barycentric.z);
					// select the shading/dithering pattern based on lighting
					pattern = SHADING_PATTERNS[lround(MAX_BRIGHT_VALUE * shader.executeFragmentShader(passAttributes[PASS_ATTRIBUTE_OUT], fragmentOut))];
				}
				// this dot is added to the fill pattern
				fill |= DOT_VALUES[offsetY][offsetX];
			}
		}
	}
	// check if any dots were inside the triangle
	if(fill) {
 		// update the braille pattern stored in the frame buffer
		fb->setPattern(col, row, pattern, depth, fill);
	}
}

void RenderContext::renderIndexedTriangles(ShaderProgram& shader, VertexArrayObject& vao) {
    shader.prepare();
    Vector4 v1, v2, v3;

    DataList** passAttributes = shader.getPassBuffers();
    passAttributes[PASS_ATTRIBUTE_V1]->clear();
    passAttributes[PASS_ATTRIBUTE_V2]->clear();
    passAttributes[PASS_ATTRIBUTE_V3]->clear();
    passAttributes[PASS_ATTRIBUTE_OUT]->clear();

    for(int i = 0; i < vao.getIndicesCount(); i += 3) {
        shader.executeVertexShader(vao.getBufferData(), passAttributes[PASS_ATTRIBUTE_V1], vao.getIndices()[i + 0], v1);
        shader.executeVertexShader(vao.getBufferData(), passAttributes[PASS_ATTRIBUTE_V2], vao.getIndices()[i + 1], v2);
        shader.executeVertexShader(vao.getBufferData(), passAttributes[PASS_ATTRIBUTE_V3], vao.getIndices()[i + 2], v3);

        v1.x /= v1.z;
        v1.y /= v1.z;

        v2.x /= v2.z;
        v2.y /= v2.z;

        v3.x /= v3.z;
        v3.y /= v3.z;
        
        r.rasterizeTriangle(v1, v2, v3, shader, passAttributes);
        passAttributes[PASS_ATTRIBUTE_V1]->clear();
        passAttributes[PASS_ATTRIBUTE_V2]->clear();
        passAttributes[PASS_ATTRIBUTE_V3]->clear();
        passAttributes[PASS_ATTRIBUTE_OUT]->clear();
    }
}

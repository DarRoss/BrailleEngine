#ifndef INCLUDE_FRAGMENT_SHADER_H
#define INCLUDE_FRAGMENT_SHADER_H

#include "Shader.h"

class FragmentShader : public Shader {
    public:
        FragmentShader(int uniformCount) :
            Shader(uniformCount)
        {}

        virtual float execute(DataList* passAttributes, Vector4& out) {
            return 1.0f;
        }

        virtual ~FragmentShader() {}
};

#endif

#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <glm/glm.hpp>
#include "common.h"

class ShadowMap {
    GLuint FramebufferName;
    GLuint depthTexture;
public:
    int width, height;

    glm::mat4 matrix;
    ShadowMap(const glm::mat4 matrix) : matrix(matrix) {}
    int init(int width, int height);
    void activate();
    void deactivate();
    void bind();
};

#endif //SHADOWMAP_H

#ifndef OPENGL_EXAMPLE_H
#define OPENGL_EXAMPLE_H

#include "../Objects/Renderer.h"

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;


class OpenGL_Example : public Renderer {
	private:
		void readTriangles(vector<Triangle>*,GLfloat*,GLfloat*,int*);
	public:
		void render(vector<Triangle>*,int*,int*,int*,int*,float*);
};

#endif 

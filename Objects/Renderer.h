#ifndef RENDERER_H
#define RENDERER_H

#include "Triangle.h"
#include <vector>

using namespace std;

class Renderer {
	public:
		virtual void render(vector<Triangle>*,int*,int*,int*,int*,float*) = 0;
};

#endif 

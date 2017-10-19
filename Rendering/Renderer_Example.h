#ifndef RENDERER_EXAMPLE_H
#define RENDERER_EXAMPLE_H

#include "../Objects/Renderer.h"
#include <iostream>

using namespace std;

class Renderer_Example : public Renderer {
	private:
		void calcPlane(double*, double*, double*, double*,
					Vertex*, Vertex*, Vertex*);
		int whichSide(Vertex*, double, double, double, Vertex*) ;
		bool correctSide(Vertex*, Vertex*, Vertex*,
					  double*, double*, double*,
					  double, double, double);
		bool planeThroughCube(Vertex*, double*, double*, double*, double, double, double);
		bool isIn(double*, double*, double*, double*, Triangle*, int, int, int);
		int tripleMin(double, double, double);
		int tripleMax(double, double, double);
		void fillTriangle(int*, int*, int*, float*, Triangle*, int*);

	public:
		void render(vector<Triangle>*,int*,int*,int*,int*,float*);
};

#endif 

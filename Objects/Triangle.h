#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Vertex.h"
#include <iostream>

using namespace std;

class Triangle {
	public:
		Vertex v1,v2,v3;
		int *color;
		double opacity;
		Triangle ();
		Triangle (const Triangle&);
		Triangle(Vertex,Vertex,Vertex); 
		void addColor(int,int,int);
		void addOpacity(double);
		void addFeatures(int,int,int,double);
		void print();
};

#endif 

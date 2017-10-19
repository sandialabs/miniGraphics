#ifndef VERTEX_H
#define VERTEX_H

#include <iostream>
using namespace std;

class Vertex {
	public:
		double p1,p2,p3;
		Vertex();
		Vertex(double,double,double);
		Vertex(const Vertex&);
		Vertex (const Vertex&,double,double,double);
		void print ();
		void getVector(Vertex*,double[]);
		void getVector(double,double,double,double*);
		double dotProduct(Vertex*);
};

#endif

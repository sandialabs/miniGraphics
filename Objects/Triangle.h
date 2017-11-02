// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

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

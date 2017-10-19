#ifndef ICET_EXAMPLE_H
#define ICET_EXAMPLE_H

#include "../Objects/Composition.h"
#include <GL/gl.h>
#include <GL/glut.h>

#include <IceT.h>
#include <IceTDevImage.h>
#include <IceTMPI.h>

#include <time.h>
#include <iostream>
using namespace std;

class IceT_Example : public Composition {
	private:
		
	public:
		void composition(int, int, int**, int**, int**, float**, int*, int*, int*, float*);
};
#endif

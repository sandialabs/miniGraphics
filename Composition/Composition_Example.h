#ifndef COMPOSITION_EXAMPLE_H
#define COMPOSITION_EXAMPLE_H

#include "../Objects/Composition.h"
#include <time.h>
#include <iostream>
using namespace std;

class Composition_Example : public Composition {
	private:
		
	public:
		void composition(int, int, int**, int**, int**, float**, int*, int*, int*, float*);
};
#endif

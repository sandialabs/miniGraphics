// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef COMPOSITION_H
#define COMPOSITION_H

using namespace std;

class Composition {
	public:
		virtual void composition(int, int, int**, int**, int**, float**, int*, int*, int*, float*) = 0;
};

#endif 

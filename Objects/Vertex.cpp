#include "Vertex.h"

Vertex::Vertex () {
}

Vertex::Vertex (double f1, double f2, double f3) {
	p1 = f1;
	p2 = f2;
	p3 = f3;
}

Vertex::Vertex (const Vertex& other) {
	p1 = other.p1;
	p2 = other.p2;
	p3 = other.p3;
}

Vertex::Vertex (const Vertex& other,double f1, double f2, double f3) {
	p1 = other.p1 + f1;
	p2 = other.p2 + f2;
	p3 = other.p3 + f3;
}

void Vertex::print () {
	cout << p1 << ',' << p2 << ',' << p3 << endl;
}

void Vertex::getVector(Vertex* other,double result[]) {
	result[0] = other->p1 - p1;
	result[1] = other->p2 - p2;
	result[2] = other->p3 - p3;
}

void Vertex::getVector(double first, double second, double third, double* result) {
	result[0] = first - p1;
	result[1] = second - p2;
	result[2] = third - p3;
}

double Vertex::dotProduct(Vertex* other) {
	return p1 * other->p1 + p2 * other->p2 + p3 * other->p3;
}

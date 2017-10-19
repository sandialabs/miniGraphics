#ifndef READDATA_H
#define READDATA_H

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <vector>

#include "../Objects/Triangle.h"

#include <iostream>
#include <fstream>
using namespace std;

vector<Triangle> readData (char s[],int resolution[3]);

#endif

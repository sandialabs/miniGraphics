// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef READDATA_H
#define READDATA_H

#include "../Objects/Mesh.hpp"

#include <string>

bool readData(const std::string& filename,
              Mesh& mesh,
              int& imageWidth,
              int& imageHeight);

#endif

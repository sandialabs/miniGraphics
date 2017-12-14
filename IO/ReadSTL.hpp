// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef READSTL_HPP
#define READSTL_HPP

#include "../Objects/Mesh.hpp"

#include <string>

bool ReadSTL(const std::string& filename, Mesh& mesh);

#endif  // READSTL_HPP

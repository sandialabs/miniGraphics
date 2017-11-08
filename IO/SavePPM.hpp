// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef SAVEPPM_HPP
#define SAVEPPM_HPP

#include "../Objects/Image.hpp"

#include <string>

/// \brief Saves the given Image data to a PPM file
///
/// Returns true if the save is successful, false otherwise.
bool SavePPM(const Image& image, const std::string& filename);

#endif  // SAVEPPM_HPP

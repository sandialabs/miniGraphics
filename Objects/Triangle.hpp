// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "Color.hpp"

#include <glm/vec3.hpp>

struct Triangle {
  glm::vec3 vertex[3];
  Color color;

  Triangle(const glm::vec3& v0 = glm::vec3(0, 0, 0),
           const glm::vec3& v1 = glm::vec3(0, 0, 0),
           const glm::vec3& v2 = glm::vec3(0, 0, 0),
           const Color& c = Color(1, 1, 1, 1))
      : vertex{v0, v1, v2}, color(c) {}

  Triangle(const float v0[3], const float v1[3], const float v2[3])
      : vertex{glm::vec3(v0[0], v0[1], v0[2]),
               glm::vec3(v1[0], v1[1], v1[2]),
               glm::vec3(v2[0], v2[1], v2[2])},
        color(1, 1, 1, 1) {}

  Triangle(const float v0[3],
           const float v1[3],
           const float v2[3],
           const float c[4])
      : vertex{glm::vec3(v0[0], v0[1], v0[2]),
               glm::vec3(v1[0], v1[1], v1[2]),
               glm::vec3(v2[0], v2[1], v2[2])},
        color(c) {}
};

#endif

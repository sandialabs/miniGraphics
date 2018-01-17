// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef PAINTER_SIMPLE_H
#define PAINTER_SIMPLE_H

#include <iostream>

#include "Painter.hpp"

#include <glm/vec3.hpp>

class PainterSimple : public Painter {
 private:
  void fillLine(Image* image,
                int y,
                const glm::vec3& edgeDir1,
                const glm::vec3& edgeBase1,
                const glm::vec3& edgeDir2,
                const glm::vec3& edgeBase2,
                const Color& color);
  void fillTriangle(Image* image,
                    const Triangle& triangle,
                    const glm::mat4& modelview,
                    const glm::mat4& projection,
                    const glm::mat3& normalTransform);

 public:
  void paint(const Mesh& mesh,
             Image* image,
             const glm::mat4& modelview,
             const glm::mat4& projection) final;
};

#endif  // PAINTER_SIMPLE_H

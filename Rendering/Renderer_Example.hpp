// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef RENDERER_EXAMPLE_H
#define RENDERER_EXAMPLE_H

#include <iostream>

#include "../Objects/Renderer.hpp"

#include <glm/vec3.hpp>

class Renderer_Example : public Renderer {
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
                    const glm::mat4x4& modelview,
                    const glm::mat4x4& projection);

 public:
  void render(const Mesh& mesh,
              Image* image,
              const glm::mat4x4& modelview,
              const glm::mat4x4& projection) final;
};

#endif

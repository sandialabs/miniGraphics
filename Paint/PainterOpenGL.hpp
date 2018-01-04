// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef PAINTER_OPENGL_H
#define PAINTER_OPENGL_H

#include "Painter.hpp"

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

class PainterOpenGL : public Painter {
 private:
  void readTriangles(const Mesh& mesh,
                     std::vector<GLfloat>& vBuffer,
                     std::vector<GLfloat>& cBuffer);

  GLFWwindow* window;
  GLuint programID;

 public:
  PainterOpenGL();
  ~PainterOpenGL();

  void paint(const Mesh& mesh,
             Image* image,
             const glm::mat4x4& modelview,
             const glm::mat4x4& projection) final;
};

#endif  // PAINTER_OPENGL_H

// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

// TODO: THIS CODE IS BROKEN RIGHT NOW. THE OPENGL RENDERING DOES NOT WORK.
// Someone needs to fix this in order for readings to be valid.

#include "PainterOpenGL.hpp"

#include <iostream>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "OpenGL_common/shader.hpp"

#include <Common/ImageRGBAFloatColorOnly.hpp>
#include <Common/ImageRGBAUByteColorFloatDepth.hpp>
#include <Common/ImageRGBAUByteColorOnly.hpp>
#include <Common/ImageRGBFloatColorDepth.hpp>

struct PainterOpenGL::Internals {
  void readTriangles(const Mesh& mesh,
                     std::vector<GLfloat>& vBuffer,
                     std::vector<GLfloat>& nBuffer,
                     std::vector<GLfloat>& cBuffer);

  GLFWwindow* window;
  GLuint programID;
};

void PainterOpenGL::Internals::readTriangles(const Mesh& mesh,
                                             std::vector<GLfloat>& vBuffer,
                                             std::vector<GLfloat>& nBuffer,
                                             std::vector<GLfloat>& cBuffer) {
  int numTriangles = mesh.getNumberOfTriangles();

  vBuffer.resize(numTriangles * 3 * 3);
  nBuffer.resize(numTriangles * 3 * 3);
  cBuffer.resize(numTriangles * 3 * 3);

  for (int triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex) {
    Triangle triangle = mesh.getTriangle(triangleIndex);

    vBuffer[triangleIndex * 9 + 0] = triangle.vertex[0].x;
    vBuffer[triangleIndex * 9 + 1] = triangle.vertex[0].y;
    vBuffer[triangleIndex * 9 + 2] = triangle.vertex[0].z;

    vBuffer[triangleIndex * 9 + 3] = triangle.vertex[1].x;
    vBuffer[triangleIndex * 9 + 4] = triangle.vertex[1].y;
    vBuffer[triangleIndex * 9 + 5] = triangle.vertex[1].z;

    vBuffer[triangleIndex * 9 + 6] = triangle.vertex[2].x;
    vBuffer[triangleIndex * 9 + 7] = triangle.vertex[2].y;
    vBuffer[triangleIndex * 9 + 8] = triangle.vertex[2].z;

    nBuffer[triangleIndex * 9 + 0] = triangle.normal.x;
    nBuffer[triangleIndex * 9 + 1] = triangle.normal.y;
    nBuffer[triangleIndex * 9 + 2] = triangle.normal.z;

    nBuffer[triangleIndex * 9 + 3] = triangle.normal.x;
    nBuffer[triangleIndex * 9 + 4] = triangle.normal.y;
    nBuffer[triangleIndex * 9 + 5] = triangle.normal.z;

    nBuffer[triangleIndex * 9 + 6] = triangle.normal.x;
    nBuffer[triangleIndex * 9 + 7] = triangle.normal.y;
    nBuffer[triangleIndex * 9 + 8] = triangle.normal.z;

    cBuffer[triangleIndex * 9 + 0] = triangle.color.Components[0];
    cBuffer[triangleIndex * 9 + 1] = triangle.color.Components[1];
    cBuffer[triangleIndex * 9 + 2] = triangle.color.Components[2];

    cBuffer[triangleIndex * 9 + 3] = triangle.color.Components[0];
    cBuffer[triangleIndex * 9 + 4] = triangle.color.Components[1];
    cBuffer[triangleIndex * 9 + 5] = triangle.color.Components[2];

    cBuffer[triangleIndex * 9 + 6] = triangle.color.Components[0];
    cBuffer[triangleIndex * 9 + 7] = triangle.color.Components[1];
    cBuffer[triangleIndex * 9 + 8] = triangle.color.Components[2];
  }
}

PainterOpenGL::PainterOpenGL() : internals(new Internals) {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    exit(1);
  }

  glfwWindowHint(GLFW_SAMPLES, 0);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,
                 GL_TRUE);  // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_ALPHA_BITS, 8);

  // Open a window and create its OpenGL context
  this->internals->window =
      glfwCreateWindow(100, 100, "miniGraphics", NULL, NULL);
  if (this->internals->window == NULL) {
    std::cerr << "Failed to open GLFW window." << std::endl;
    glfwTerminate();
    exit(1);
  }
  glfwMakeContextCurrent(this->internals->window);

  // Initialize GLEW
  glewExperimental = true;  // Needed for core profile
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    glfwTerminate();
    exit(1);
  }

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(this->internals->window, GLFW_STICKY_KEYS, GL_TRUE);

  // Create and compile our GLSL program from the shaders
  this->internals->programID = LoadShaders();
}

PainterOpenGL::~PainterOpenGL() {
  glfwMakeContextCurrent(this->internals->window);
  glDeleteProgram(this->internals->programID);

  // Close OpenGL window and terminate GLFW
  glfwDestroyWindow(this->internals->window);
  glfwTerminate();

  delete this->internals;
}

void PainterOpenGL::paint(const Mesh& mesh,
                          ImageFull& image,
                          const glm::mat4x4& modelview,
                          const glm::mat4x4& projection) {
  int windowWidth = image.getWidth();
  int windowHeight = image.getHeight();

  glfwSetWindowSize(this->internals->window, windowWidth, windowHeight);
  glfwMakeContextCurrent(this->internals->window);

  // Black background
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);

  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  // Get a handle for our "MVP" uniform
  GLuint MVDMatrixId = glGetUniformLocation(this->internals->programID, "MVP");

  glm::mat4 MVP = projection * modelview;

  // Get a handle for our "normalTransform" uniform
  GLuint normalTransformID =
      glGetUniformLocation(this->internals->programID, "normalTransform");

  glm::mat3 normalTransform = glm::inverseTranspose(glm::mat3(modelview));

  // Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive
  // vertices give a triangle. We could probably speed things up by using the
  // indices in the mesh directly, but that is for future work.
  std::vector<GLfloat> vertexBufferData;

  // One normal for each vertex.
  std::vector<GLfloat> normalBufferData;

  // One color for each vertex.
  std::vector<GLfloat> colorBufferData;

  this->internals->readTriangles(
      mesh, vertexBufferData, normalBufferData, colorBufferData);

  GLuint vertexbuffer;
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER,
               vertexBufferData.size() * sizeof(float),
               &vertexBufferData.front(),
               GL_STATIC_DRAW);

  GLuint normalbuffer;
  glGenBuffers(1, &normalbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
  glBufferData(GL_ARRAY_BUFFER,
               normalBufferData.size() * sizeof(float),
               &normalBufferData.front(),
               GL_STATIC_DRAW);

  GLuint colorbuffer;
  glGenBuffers(1, &colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER,
               colorBufferData.size() * sizeof(float),
               &colorBufferData.front(),
               GL_STATIC_DRAW);

  // ---------------------------------------------
  // Render to Texture - specific code begins here
  // ---------------------------------------------
  // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth
  // buffer.
  GLuint FramebufferName = 0;
  glGenFramebuffers(1, &FramebufferName);
  glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

  // The texture we're going to render to
  GLuint renderedTexture;
  glGenTextures(1, &renderedTexture);

  // "Bind" the newly created texture : all future texture functions will modify
  // this texture
  glBindTexture(GL_TEXTURE_2D, renderedTexture);

  // Give an empty image to OpenGL ( the last "0" means "empty" )
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               windowWidth,
               windowHeight,
               0,
               GL_RGBA,
               GL_FLOAT,
               0);

  // Poor filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // The depth buffer
  GLuint depthrenderbuffer;
  glGenRenderbuffers(1, &depthrenderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
  glRenderbufferStorage(
      GL_RENDERBUFFER, GL_DEPTH_COMPONENT, windowWidth, windowHeight);
  glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

  // Set "renderedTexture" as our colour attachement #0
  glFramebufferTexture(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

  // Set the list of draw buffers.
  GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, DrawBuffers);  // "1" is the size of DrawBuffers

  // Always check that our framebuffer is ok
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Failed to create framebuffer." << std::endl;
    return;
  }

  // clang-format off
  // The fullscreen quad's FBO
  static const GLfloat quadVertexBufferData[] = {
      -1.0f, -1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,
      -1.0f, 1.0f, 0.0f,
      -1.0f, 1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,
      1.0f, 1.0f, 0.0f,
  };
  // clang-format on

  GLuint quad_vertexbuffer;
  glGenBuffers(1, &quad_vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(quadVertexBufferData),
               quadVertexBufferData,
               GL_STATIC_DRAW);

  // Create and compile our GLSL program from the shaders
  GLuint quad_programID = LoadShaders();
  // GLuint texID =
  glGetUniformLocation(quad_programID, "renderedTexture");
  // GLuint timeID =
  glGetUniformLocation(quad_programID, "time");

  glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
  glViewport(0, 0, windowWidth, windowHeight);  // Render on the whole
                                                // framebuffer, complete from
                                                // the lower left corner to the
                                                // upper right

  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Use our shader
  glUseProgram(this->internals->programID);

  // Send our transformation to the currently bound shader,
  // in the "MVP" and "normalTransform" uniforms
  glUniformMatrix4fv(MVDMatrixId, 1, GL_FALSE, &MVP[0][0]);
  glUniformMatrix3fv(normalTransformID, 1, GL_FALSE, &normalTransform[0][0]);

  // 1st attribute buffer : vertices
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glVertexAttribPointer(0,  // attribute. No particular reason for 0, but must
                            // match the layout in the shader.
                        3,  // size
                        GL_FLOAT,  // type
                        GL_FALSE,  // normalized?
                        0,         // stride
                        (void*)0   // array buffer offset
                        );

  // 2nd attribute buffer : normals
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
  glVertexAttribPointer(1,  // attribute. No particular reason for 1, but must
                            // match the layout in the shader.
                        3,  // size
                        GL_FLOAT,  // type
                        GL_FALSE,  // normalized?
                        0,         // stride
                        (void*)0   // array buffer offset
                        );

  // 3rd attribute buffer : colors
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glVertexAttribPointer(2,  // attribute. No particular reason for 2, but must
                            // match the layout in the shader.
                        3,  // size
                        GL_FLOAT,  // type
                        GL_FALSE,  // normalized?
                        0,         // stride
                        (void*)0   // array buffer offset
                        );

  // Enable alpha blending
  glEnable(GL_BLEND);
  // Set the blending function for back-to-front. Note that our colors are
  // premultiplied by alpha. This is important to make sure we have the right
  // alpha in the imagebuffer.
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Draw the triangles !
  glDrawArrays(GL_TRIANGLES, 0, mesh.getNumberOfTriangles() * 3);

  glDisable(GL_BLEND);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);

  glFlush();
  glReadBuffer(GL_BACK);

  ImageRGBAUByteColorFloatDepth* rgbaByteFloatImage =
      dynamic_cast<ImageRGBAUByteColorFloatDepth*>(&image);
  ImageRGBFloatColorDepth* rgbFloatFloatImage =
      dynamic_cast<ImageRGBFloatColorDepth*>(&image);
  ImageRGBAUByteColorOnly* rgbaByteImage =
      dynamic_cast<ImageRGBAUByteColorOnly*>(&image);
  ImageRGBAFloatColorOnly* rgbaFloatImage =
      dynamic_cast<ImageRGBAFloatColorOnly*>(&image);
  if (rgbaByteFloatImage != nullptr) {
    glReadPixels(0,
                 0,
                 windowWidth,
                 windowHeight,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 rgbaByteFloatImage->getColorBuffer());
    glReadPixels(0,
                 0,
                 windowWidth,
                 windowHeight,
                 GL_DEPTH_COMPONENT,
                 GL_FLOAT,
                 rgbaByteFloatImage->getDepthBuffer());
  } else if (rgbFloatFloatImage != nullptr) {
    glReadPixels(0,
                 0,
                 windowWidth,
                 windowHeight,
                 GL_RGB,
                 GL_FLOAT,
                 rgbFloatFloatImage->getColorBuffer());
    glReadPixels(0,
                 0,
                 windowWidth,
                 windowHeight,
                 GL_DEPTH_COMPONENT,
                 GL_FLOAT,
                 rgbFloatFloatImage->getDepthBuffer());
  } else if (rgbaByteImage != nullptr) {
    glReadPixels(0,
                 0,
                 windowWidth,
                 windowHeight,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 rgbaByteImage->getColorBuffer());
  } else if (rgbaFloatImage != nullptr) {
    glReadPixels(0,
                 0,
                 windowWidth,
                 windowHeight,
                 GL_RGBA,
                 GL_FLOAT,
                 rgbaFloatImage->getColorBuffer());
  } else {
    std::cerr << "Image type not supported for OpenGL." << std::endl;
    exit(1);
  }

  // Swap buffers
  //		glfwPollEvents();
  //		glfwSwapBuffers(this->window);

  // Cleanup VBO and shader
  glDeleteBuffers(1, &vertexbuffer);
  glDeleteBuffers(1, &normalbuffer);
  glDeleteBuffers(1, &colorbuffer);
  glDeleteVertexArrays(1, &VertexArrayID);

  glDeleteFramebuffers(1, &FramebufferName);
  glDeleteTextures(1, &renderedTexture);
  glDeleteRenderbuffers(1, &depthrenderbuffer);
  glDeleteBuffers(1, &quad_vertexbuffer);
  glDeleteVertexArrays(1, &VertexArrayID);
}

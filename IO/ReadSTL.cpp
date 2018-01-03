// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.


#include "ReadSTL.hpp"

#include <fstream>
#include <string>

static glm::vec3 ReadAsciiVector(std::ifstream& file) {
  glm::vec3 vec;
  file >> vec[0];
  file >> vec[1];
  file >> vec[2];
  return vec;
}

static void ReadExpectedAsciiToken(std::ifstream& file,
                                   const std::string& expectedToken) {
  std::string readToken;
  file >> readToken;
  if (readToken != expectedToken) {
    std::cerr << "Invalid token in STL file. Expected " << expectedToken
              << ". Got " << readToken << std::endl;
    throw std::logic_error("Error reading STL ASCII file.");
  }
}

static bool ReadSTLAscii(std::ifstream& file, Mesh& mesh) {
  mesh = Mesh();

  // Skip header
  std::string line;
  std::getline(file, line);

  try {
    while (true) {
      std::string facetToken;
      file >> facetToken;
      if (facetToken == "endsolid") {
        // Reached expected end of file
        break;
      } else if (facetToken != "facet") {
        std::cerr << "Invalid token in STL file. Expected facet. Got "
                  << facetToken << std::endl;
        throw std::logic_error("Error reading STL ASCII file.");
      }
      ReadExpectedAsciiToken(file, "normal");
      glm::vec3 normal = ReadAsciiVector(file);

      ReadExpectedAsciiToken(file, "outer");
      ReadExpectedAsciiToken(file, "loop");
      for (int vertId = 0; vertId < 3; ++vertId) {
        ReadExpectedAsciiToken(file, "vertex");
        glm::vec3 vertex = ReadAsciiVector(file);
        mesh.addVertex(vertex);
      }

      int indices[3];
      indices[0] = 3 * mesh.getNumberOfTriangles() + 0;
      indices[1] = 3 * mesh.getNumberOfTriangles() + 1;
      indices[2] = 3 * mesh.getNumberOfTriangles() + 2;
      mesh.addTriangle(indices, normal);

      ReadExpectedAsciiToken(file, "endloop");
      ReadExpectedAsciiToken(file, "endfacet");
    }
  } catch (std::logic_error error) {
    std::cerr << error.what() << std::endl;
    return false;
  }

  // Sanity check to make sure the number of vertices and triangles adds up
  if (mesh.getNumberOfVertices() != 3 * mesh.getNumberOfTriangles()) {
    std::cerr << "Internal error: got wrong number of vertices/triangles."
              << std::endl;
    return false;
  }

  return true;
}

static bool ReadSTLBinary(std::ifstream& file, Mesh& mesh) {
  // Read header
  char header[80];
  file.read(header, 80);

  unsigned int numTriangles;
  file.read(reinterpret_cast<char*>(&numTriangles), sizeof(numTriangles));

  mesh = Mesh(3 * numTriangles, numTriangles);
  for (unsigned int triangleIndex = 0; triangleIndex < numTriangles;
       ++triangleIndex) {
    float data[12];
    file.read(reinterpret_cast<char*>(data), sizeof(data));
    glm::vec3 normal(data[0], data[1], data[2]);
    glm::vec3 vert0(data[3], data[4], data[5]);
    glm::vec3 vert1(data[6], data[7], data[8]);
    glm::vec3 vert2(data[9], data[10], data[11]);

    int connections[3];
    connections[0] = 3 * triangleIndex + 0;
    connections[1] = 3 * triangleIndex + 1;
    connections[2] = 3 * triangleIndex + 2;

    mesh.setVertex(3 * triangleIndex + 0, vert0);
    mesh.setVertex(3 * triangleIndex + 1, vert1);
    mesh.setVertex(3 * triangleIndex + 2, vert2);

    mesh.setTriangle(triangleIndex, connections, normal);

    // Attribute not used but needs to be skipped.
    file.ignore(2);
  }

  return !file.fail();
}

bool ReadSTL(const std::string& filename, Mesh& mesh) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return false;
  }

  // Identify whether the file is ASCII or binary
  char header[7];
  file.get(header, 7);
  if (std::string(header) == "solid ") {
    file.seekg(0);
    return ReadSTLAscii(file, mesh);
  } else {
    // Reopen file as binary
    file.close();
    file.open(filename, std::ios_base::binary);
    return ReadSTLBinary(file, mesh);
  }
}

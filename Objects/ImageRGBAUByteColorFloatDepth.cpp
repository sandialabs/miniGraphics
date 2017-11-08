// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageRGBAUByteColorFloatDepth.hpp"

#include <assert.h>

ImageRGBAUByteColorFloatDepth::ImageRGBAUByteColorFloatDepth(int _width,
                                                             int _height)
    : Image(_width, _height),
      colorBuffer(new std::vector<unsigned int>(_width * _height)),
      depthBuffer(new std::vector<float>(_width * _height)) {}

Color ImageRGBAUByteColorFloatDepth::getColor(int pixelIndex) const {
  const unsigned char *colorArray = this->getColorBuffer() + 4 * pixelIndex;

  Color color;
  color.SetComponentFromByte(0, colorArray[0]);
  color.SetComponentFromByte(1, colorArray[1]);
  color.SetComponentFromByte(2, colorArray[2]);
  color.SetComponentFromByte(3, colorArray[3]);

  return color;
}

void ImageRGBAUByteColorFloatDepth::setColor(int pixelIndex,
                                             const Color &color) {
  unsigned char *colorArray = this->getColorBuffer() + 4 * pixelIndex;

  colorArray[0] = color.GetComponentAsByte(0);
  colorArray[1] = color.GetComponentAsByte(1);
  colorArray[2] = color.GetComponentAsByte(2);
  colorArray[3] = color.GetComponentAsByte(3);
}

float ImageRGBAUByteColorFloatDepth::getDepth(int pixelIndex) const {
  return this->getDepthBuffer()[pixelIndex];
}

void ImageRGBAUByteColorFloatDepth::setDepth(int pixelIndex, float depth) {
  this->getDepthBuffer()[pixelIndex] = depth;
}

void ImageRGBAUByteColorFloatDepth::clear(const Color &color, float depth) {
  int numPixels = this->getNumberOfPixels();
  if (numPixels < 1) {
    return;
  }

  // Encode the color by calling setColor for the first pixel.
  this->setColor(0, color);

  unsigned int *cBuffer = this->getEncodedColorBuffer();
  float *dBuffer = this->getDepthBuffer();

  unsigned int encodedColor = cBuffer[0];

  for (int index = 0; index < numPixels; ++index) {
    cBuffer[index] = encodedColor;
    dBuffer[index] = depth;
  }
}

void ImageRGBAUByteColorFloatDepth::blend(const Image *_otherImage,
                                          BlendOrder) {
  const ImageRGBAUByteColorFloatDepth *otherImage =
      dynamic_cast<const ImageRGBAUByteColorFloatDepth *>(_otherImage);
  assert((otherImage != NULL) && "Attempting to blend invalid images.");

  int numPixels = this->getNumberOfPixels();
  assert((numPixels == otherImage->getNumberOfPixels()) &&
         "Attempting to blend images of different sizes.");

  unsigned int *myColorBuffer = this->getEncodedColorBuffer();
  float *myDepthBuffer = this->getDepthBuffer();
  const unsigned int *otherColorBuffer = otherImage->getEncodedColorBuffer();
  const float *otherDepthBuffer = otherImage->getDepthBuffer();

  for (int index = 0; index < numPixels; ++index) {
    if (otherDepthBuffer[index] < myDepthBuffer[index]) {
      myColorBuffer[index] = otherColorBuffer[index];
      myDepthBuffer[index] = otherDepthBuffer[index];
    } else {
      // This pixels is closer, keep its value.
    }
  }
}

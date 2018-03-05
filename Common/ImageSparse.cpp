// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "ImageSparse.hpp"

int ImageSparse::RunLengthIterator::advance(int numPixels) {
  int numActivePixels = 0;
  while (numPixels > 0) {
    if (numPixels < this->workingRegion.backgroundPixels) {
      this->workingRegion.backgroundPixels -= numPixels;
      return numActivePixels;
    } else {
      numPixels -= this->workingRegion.backgroundPixels;
      this->workingRegion.backgroundPixels = 0;
      if (numPixels < this->workingRegion.foregroundPixels) {
        this->workingRegion.foregroundPixels -= numPixels;
        return numActivePixels + numPixels;
      } else {
        numPixels -= this->workingRegion.foregroundPixels;
        numActivePixels += this->workingRegion.foregroundPixels;
        this->workingRegion.foregroundPixels = 0;
        this->updateWorkingRegion();
      }
    }
  }
  return numActivePixels;
}

void ImageSparse::RunLengthIterator::copyPixels(
    int numPixels,
    std::vector<RunLengthRegion>& outRunlengths,
    int& numActivePixels) {
  numActivePixels = 0;
  while (numPixels > 0) {
    assert(!this->atEnd());
    if (numPixels <= this->getWorkingBackground()) {
      // Only background pixels to copy over.
      outRunlengths.push_back(RunLengthRegion(numPixels, 0));
      this->workingRegion.backgroundPixels -= numPixels;
      return;
    } else if (numPixels <= this->getWorkingPixels()) {
      // Copy over all background pixels and remaining foreground pixels
      numPixels -= this->workingRegion.backgroundPixels;
      numActivePixels += numPixels;
      outRunlengths.push_back(
          RunLengthRegion(this->workingRegion.backgroundPixels, numPixels));
      this->workingRegion.backgroundPixels = 0;
      this->workingRegion.foregroundPixels -= numPixels;
      return;
    } else {
      // Copy over entire run length and iterate to next one
      outRunlengths.push_back(this->workingRegion);
      numActivePixels += this->getWorkingForeground();
      numPixels -= this->getWorkingPixels();
      this->workingRegion = *this->currentRegion;
      ++this->currentRegion;
    }
  }
}

void ImageSparse::copyRunlengthRegion(
    int subregionBegin,
    int subregionEnd,
    std::vector<RunLengthRegion>& targetRunLengths,
    int& activeSubregionBegin,
    int& activeSubregionEnd) const {
  assert(subregionBegin <= subregionEnd);
  assert(subregionBegin >= 0);
  assert(subregionEnd <= this->getNumberOfPixels());

  RunLengthIterator inRunLength = this->createRunLengthIterator();

  // Skip over run lengths before subregionBegin
  activeSubregionBegin = inRunLength.advance(subregionBegin);

  // Copy run lengths over the range
  int numActiveToCopy;
  inRunLength.copyPixels(
      subregionEnd - subregionBegin, targetRunLengths, numActiveToCopy);

  activeSubregionEnd = activeSubregionBegin + numActiveToCopy;
}

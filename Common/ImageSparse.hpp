// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGESPARSE_HPP
#define IMAGESPARSE_HPP

#include "Image.hpp"

class ImageFull;

class ImageSparse : public Image {
 protected:
  struct RunLengthRegion {
    int backgroundPixels;
    int foregroundPixels;

    RunLengthRegion(int _backgroundPixels = 0, int _foregroundPixels = 0)
        : backgroundPixels(_backgroundPixels),
          foregroundPixels(_foregroundPixels) {}
  };

  std::shared_ptr<std::vector<RunLengthRegion>> runLengths;

  class RunLengthIterator {
    std::vector<RunLengthRegion>::const_iterator currentRegion;
    std::vector<RunLengthRegion>::const_iterator endRegion;
    RunLengthRegion workingRegion;

    void updateWorkingRegion() {
      while ((this->workingRegion.backgroundPixels == 0) &&
             (this->workingRegion.foregroundPixels == 0) &&
             (this->currentRegion != this->endRegion)) {
        this->workingRegion = *this->currentRegion;
        ++this->currentRegion;
      }
    }

    inline void verify() const {
      assert((this->currentRegion == this->endRegion) ||
             (this->workingRegion.backgroundPixels > 0) ||
             (this->workingRegion.foregroundPixels > 0));
    }

   public:
    RunLengthIterator(const std::vector<RunLengthRegion>& runLengths)
        : currentRegion(runLengths.begin()), endRegion(runLengths.end()) {
      this->updateWorkingRegion();
    }

    bool atEnd() const {
      return ((this->workingRegion.backgroundPixels == 0) &&
              (this->workingRegion.foregroundPixels == 0) &&
              (this->currentRegion == this->endRegion));
    }

    bool inBackground() const {
      return (this->workingRegion.backgroundPixels > 0);
    }

    bool inForeground() const {
      return !inBackground() && (this->workingRegion.foregroundPixels > 0);
    }

    const RunLengthRegion& getWorkingRegion() const {
      return this->workingRegion;
    }

    int getWorkingBackground() const {
      return this->getWorkingRegion().backgroundPixels;
    }

    int getWorkingForeground() const {
      return this->getWorkingRegion().foregroundPixels;
    }

    int getWorkingPixels() const {
      return this->getWorkingBackground() + this->getWorkingForeground();
    }

    // Returns the number of active pixels that were skipped over.
    int advance(int numPixels);

    void copyPixels(int numPixels,
                    std::vector<RunLengthRegion>& outRunlengths,
                    int& numActivePixels);
  };

  ImageSparse(int _width, int _height, int _regionBegin, int _regionEnd)
      : Image(_width, _height, _regionBegin, _regionEnd),
        runLengths(new std::vector<RunLengthRegion>) {}

  ImageSparse(int _width,
              int _height,
              int _regionBegin,
              int _regionEnd,
              std::shared_ptr<std::vector<RunLengthRegion>> _runLengths)
      : Image(_width, _height, _regionBegin, _regionEnd),
        runLengths(_runLengths) {}

  RunLengthIterator createRunLengthIterator() const {
    return RunLengthIterator(*this->runLengths);
  }

  // Resizes arrays based on the runlengths. Arrays might be larger than
  // necessary because lengths were not known a priori.
  template <typename PixelStorageType>
  void shrinkArraysImpl(PixelStorageType& pixelStorage) const {
    // Count inactive/active pixels
    int totalInactivePixels = 0;
    int totalActivePixels = 0;
    int runLengthSize = 0;
    for (; runLengthSize < this->runLengths->size(); ++runLengthSize) {
      const RunLengthRegion& runLength = this->runLengths->at(runLengthSize);
      if ((runLength.backgroundPixels == 0) &&
          (runLength.foregroundPixels == 0)) {
        break;
      }
      totalInactivePixels += runLength.backgroundPixels;
      totalActivePixels += runLength.foregroundPixels;
    }

    assert((totalInactivePixels + totalActivePixels) ==
           this->getNumberOfPixels());
    assert(totalActivePixels <= pixelStorage.getNumberOfPixels());
    if (totalActivePixels < pixelStorage.getNumberOfPixels()) {
      // Only resize if you are actually shrinking the array. If you have a
      // shared storage (i.e. from calling window), that can mess up storage
      // for other objects. You should only have to shrink the array when you
      // created it larger than necessary.
      pixelStorage.resizeBuffers(0, totalActivePixels);
    }
    this->runLengths->resize(runLengthSize);
  }

  void copyRunlengthRegion(int subregionBegin,
                           int subregionEnd,
                           std::vector<RunLengthRegion>& targetRunLengths,
                           int& activeSubregionBegin,
                           int& activeSubregionEnd) const;

 public:
  virtual std::unique_ptr<ImageFull> uncompress() const = 0;
};

#endif  // IMAGESPARSE_HPP

// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGESPARSECOLORONLY_HPP
#define IMAGESPARSECOLORONLY_HPP

#include "ImageSparse.hpp"

#include "ImageColorOnly.hpp"

#include <algorithm>

template <typename Features>
class ImageSparseColorOnly : public ImageSparse {
 public:
  using ColorType = typename Features::ColorType;
  static constexpr int ColorVecSize = Features::ColorVecSize;

 private:
  using ThisType = ImageSparseColorOnly<Features>;
  using StorageType = ImageColorOnly<Features>;

  std::shared_ptr<StorageType> pixelStorage;

  static constexpr int BACKGROUND_TAG = 89016;
  static constexpr int RUN_LENGTHS_TAG = 89017;

  struct BackgroundInfo {
    ColorType color[ColorVecSize];
  };
  BackgroundInfo background;
  void setBackground(const Color& color) {
    Features::encodeColor(color, this->background.color);
  }

  ImageSparseColorOnly(
      int _width,
      int _height,
      int _regionBegin,
      int _regionEnd,
      const Viewport& _validViewport,
      std::shared_ptr<StorageType> _pixelStorage,
      std::shared_ptr<std::vector<RunLengthRegion>> _runLengths,
      const BackgroundInfo& _background)
      : ImageSparse(_width,
                    _height,
                    _regionBegin,
                    _regionEnd,
                    _validViewport,
                    _runLengths),
        pixelStorage(_pixelStorage),
        background(_background) {}

 public:
  ImageSparseColorOnly(const StorageType& toCompress)
      : ImageSparse(toCompress.getWidth(),
                    toCompress.getHeight(),
                    toCompress.getRegionBegin(),
                    toCompress.getRegionEnd(),
                    toCompress.getValidViewport()) {
    std::unique_ptr<Image> imageBuffer = toCompress.createNew(
        this->getWidth(),
        this->getHeight(),
        0,
        0,
        Viewport(0, 0, this->getWidth() - 1, this->getHeight() - 1));
    StorageType* pixelStorageP = dynamic_cast<StorageType*>(imageBuffer.get());
    if (pixelStorageP != nullptr) {
      imageBuffer.release();
      this->pixelStorage.reset(pixelStorageP);
    } else {
      throw std::runtime_error(
          "ImageSparseColorOnly called with bad image type.");
    }
    this->setBackground(Color(0, 0, 0, 0));
    this->compress(toCompress);
  }

 private:
  bool isBackground(const ColorType colorComponents[ColorVecSize]) const {
    // Might want a more sophisticated way to check for background if we run
    // into issues like numeric instability.
    for (int i = 0; i < ColorVecSize; ++i) {
      if (colorComponents[i] != this->background.color[i]) {
        return false;
      }
    }
    return true;
  }

  void compress(const StorageType& toCompress) {
    int numActivePixels = 0;
    int iPixel;
    this->runLengths->resize(0);
    RunLengthRegion workingRunLength;
    const Viewport& validViewport = toCompress.getValidViewport();

    // There is currently little reason to compress an image with a range
    // narrower than the full image, and the implementation would add
    // complication here. For now, I am disallowing that.
    if ((toCompress.getRegionBegin() > 0) ||
        (toCompress.getRegionEnd() <
         (toCompress.getWidth() * toCompress.getHeight()))) {
      std::cerr << "Compression of subregion images currently not supported"
                << std::endl;
      abort();
    }

    // Skip over pixels at the bottom of the image
    workingRunLength.backgroundPixels =
        validViewport.getMinY() * toCompress.getWidth();
    iPixel = workingRunLength.backgroundPixels;

    for (int y = validViewport.getMinY(); y <= validViewport.getMaxY(); ++y) {
      if (validViewport.getMinX() > 0) {
        // Skip pixels at left of the image
        if (workingRunLength.foregroundPixels > 0) {
          this->runLengths->push_back(workingRunLength);
          workingRunLength = RunLengthRegion();
        }
        int numToSkip = validViewport.getMinX();
        workingRunLength.backgroundPixels += numToSkip;
        iPixel += numToSkip;
      }
      int x = validViewport.getMinX();
      while (x <= validViewport.getMaxX()) {
        if (workingRunLength.foregroundPixels == 0) {
          while ((x <= validViewport.getMaxX()) &&
                 this->isBackground(toCompress.getColorBuffer(iPixel))) {
            ++workingRunLength.backgroundPixels;
            ++x;
            ++iPixel;
          }
        }
        while ((x <= validViewport.getMaxX()) &&
               !this->isBackground(toCompress.getColorBuffer(iPixel))) {
          ++workingRunLength.foregroundPixels;
          ++x;
          ++iPixel;
          ++numActivePixels;
        }
        if (x <= validViewport.getMaxX()) {
          this->runLengths->push_back(workingRunLength);
          workingRunLength = RunLengthRegion();
        }
      }
      if (validViewport.getMaxX() < toCompress.getWidth() - 1) {
        // Skip pixels at right of the image
        if (workingRunLength.foregroundPixels > 0) {
          this->runLengths->push_back(workingRunLength);
          workingRunLength = RunLengthRegion();
        }
        int numToSkip = toCompress.getWidth() - validViewport.getMaxX() - 1;
        workingRunLength.backgroundPixels += numToSkip;
        iPixel += numToSkip;
      }
    }

    // Skip over pixels at top of the image
    if (validViewport.getMaxY() < toCompress.getHeight() - 1) {
      if (workingRunLength.foregroundPixels > 0) {
        this->runLengths->push_back(workingRunLength);
        workingRunLength = RunLengthRegion();
      }
      int numToSkip = (toCompress.getHeight() - validViewport.getMaxY() - 1) *
                      toCompress.getWidth();
      workingRunLength.backgroundPixels += numToSkip;
    }

    this->runLengths->push_back(workingRunLength);

    this->pixelStorage->resizeBuffers(0, numActivePixels);

    iPixel = 0;
    int iActivePixel = 0;
    for (auto&& runLength : *this->runLengths) {
      iPixel += runLength.backgroundPixels;
      if (runLength.foregroundPixels > 0) {
        std::copy(
            toCompress.getColorBuffer(iPixel),
            toCompress.getColorBuffer(iPixel + runLength.foregroundPixels),
            this->pixelStorage->getColorBuffer(iActivePixel));
        iActivePixel += runLength.foregroundPixels;
        iPixel += runLength.foregroundPixels;
      }
    }

    // Sanity checks
    assert(iPixel == this->getNumberOfPixels());
    assert(iActivePixel == this->pixelStorage->getNumberOfPixels());
    this->shrinkArrays();
  }

  // Clears the image using the background information already captured.
  void clearKnownBackground() {
    this->runLengths->resize(1);
    this->runLengths->at(0).backgroundPixels = this->getNumberOfPixels();
    this->runLengths->at(0).foregroundPixels = 0;
    this->pixelStorage->resizeBuffers(0, 0);
  }

  // Resizes arrays based on the runlengths. Arrays might be larger than
  // necessary because lengths were not known a priori.
  void shrinkArrays() const { this->shrinkArraysImpl(*this->pixelStorage); }

 public:
  std::unique_ptr<Image> blend(const Image& _otherImage) const final {
    const ThisType* otherImage = dynamic_cast<const ThisType*>(&_otherImage);
    assert((otherImage != NULL) && "Attempting to blend invalid images.");

    const ThisType* topImage = this;
    const ThisType* bottomImage = otherImage;

    topImage->shrinkArrays();
    bottomImage->shrinkArrays();

    if ((topImage->getRegionBegin() == bottomImage->getRegionBegin()) &&
        (topImage->getRegionEnd() == bottomImage->getRegionEnd())) {
      if (topImage->pixelStorage->getNumberOfPixels() < 1) {
        return bottomImage->deepCopy();
      }
      if (bottomImage->pixelStorage->getNumberOfPixels() < 1) {
        return topImage->deepCopy();
      }
    }

    int totalRegionBegin =
        std::min(topImage->getRegionBegin(), bottomImage->getRegionBegin());
    int totalRegionEnd =
        std::max(topImage->getRegionEnd(), bottomImage->getRegionEnd());

    int maxNumActivePixels =
        std::min(topImage->pixelStorage->getNumberOfPixels() +
                     bottomImage->pixelStorage->getNumberOfPixels(),
                 totalRegionEnd - totalRegionBegin);

    const ColorType* topColorBuffer = topImage->pixelStorage->getColorBuffer();
    const ColorType* bottomColorBuffer =
        bottomImage->pixelStorage->getColorBuffer();

    std::unique_ptr<Image> outImageHolder = this->createNew(
        this->getWidth(),
        this->getHeight(),
        totalRegionBegin,
        totalRegionEnd,
        this->getValidViewport().unionWith(otherImage->getValidViewport()));
    ThisType* outImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((outImage != NULL) && "Internal error: createNew bad type.");
    outImage->pixelStorage->resizeBuffers(0, maxNumActivePixels);
    outImage->runLengths->resize(0);
    ColorType* outColorBuffer = outImage->pixelStorage->getColorBuffer();

    RunLengthIterator topRunLength = topImage->createRunLengthIterator();
    RunLengthIterator bottomRunLength = bottomImage->createRunLengthIterator();

    // Manage where part of one image has a region that starts before the other
    if (topImage->getRegionBegin() < bottomImage->getRegionBegin()) {
      int numToCopy =
          bottomImage->getRegionBegin() - topImage->getRegionBegin();
      int numActivePixels;
      topRunLength.copyPixels(
          numToCopy, *outImage->runLengths, numActivePixels);
      std::copy(topColorBuffer,
                topColorBuffer + numActivePixels * ColorVecSize,
                outColorBuffer);
      topColorBuffer += numActivePixels * ColorVecSize;
      outColorBuffer += numActivePixels * ColorVecSize;
    } else if (bottomImage->getRegionBegin() < topImage->getRegionBegin()) {
      int numToCopy =
          topImage->getRegionBegin() - bottomImage->getRegionBegin();
      int numActivePixels;
      bottomRunLength.copyPixels(
          numToCopy, *outImage->runLengths, numActivePixels);
      std::copy(bottomColorBuffer,
                bottomColorBuffer + numActivePixels * ColorVecSize,
                outColorBuffer);
      bottomColorBuffer += numActivePixels * ColorVecSize;
      outColorBuffer += numActivePixels * ColorVecSize;
    } else {
      outImage->runLengths->push_back(RunLengthRegion());
    }

    // Blend where the two images intersect
    while (!topRunLength.atEnd() && !bottomRunLength.atEnd()) {
      if (topRunLength.inBackground() && bottomRunLength.inBackground()) {
        // Case 1: Both images are in background. Just add to inactive count.
        if (outImage->runLengths->back().foregroundPixels != 0) {
          outImage->runLengths->push_back(RunLengthRegion());
        }
        int numInactive = std::min(topRunLength.getWorkingBackground(),
                                   bottomRunLength.getWorkingBackground());
        outImage->runLengths->back().backgroundPixels += numInactive;
        topRunLength.advance(numInactive);
        bottomRunLength.advance(numInactive);
      } else if (topRunLength.inBackground()) {
        // Case 2: Top image in background, bottom image in foreground.
        int numPixels = std::min(topRunLength.getWorkingBackground(),
                                 bottomRunLength.getWorkingForeground());

        std::copy(bottomColorBuffer,
                  bottomColorBuffer + (numPixels * ColorVecSize),
                  outColorBuffer);
        bottomColorBuffer += numPixels * ColorVecSize;
        outColorBuffer += numPixels * ColorVecSize;

        topRunLength.advance(numPixels);
        bottomRunLength.advance(numPixels);
        outImage->runLengths->back().foregroundPixels += numPixels;
      } else if (bottomRunLength.inBackground()) {
        // Case 3: Bottom image in background, top image in foreground.
        int numPixels = std::min(bottomRunLength.getWorkingBackground(),
                                 topRunLength.getWorkingForeground());

        std::copy(topColorBuffer,
                  topColorBuffer + (numPixels * ColorVecSize),
                  outColorBuffer);
        topColorBuffer += numPixels * ColorVecSize;
        outColorBuffer += numPixels * ColorVecSize;

        bottomRunLength.advance(numPixels);
        topRunLength.advance(numPixels);
        outImage->runLengths->back().foregroundPixels += numPixels;
      } else {
        // Case 4: Both images are in foreground, blend them.
        int numPixels = std::min(topRunLength.getWorkingForeground(),
                                 bottomRunLength.getWorkingForeground());

        for (int pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex) {
          Features::blend(topColorBuffer + (pixelIndex * ColorVecSize),
                          bottomColorBuffer + (pixelIndex * ColorVecSize),
                          outColorBuffer + (pixelIndex * ColorVecSize));
        }

        topColorBuffer += numPixels * ColorVecSize;
        bottomColorBuffer += numPixels * ColorVecSize;
        outColorBuffer += numPixels * ColorVecSize;

        topRunLength.advance(numPixels);
        bottomRunLength.advance(numPixels);
        outImage->runLengths->back().foregroundPixels += numPixels;
      }
    }

    // Manage where part of one image has a region past the end of the other
    if (!topRunLength.atEnd()) {
      assert(bottomRunLength.atEnd());
      int numActivePixels;
      topRunLength.copyPixels(
          topImage->getRegionEnd() - bottomImage->getRegionEnd(),
          *outImage->runLengths,
          numActivePixels);
      std::copy(topColorBuffer,
                topColorBuffer + (numActivePixels * ColorVecSize),
                outColorBuffer);

      assert(topRunLength.atEnd());
    }
    if (!bottomRunLength.atEnd()) {
      assert(topRunLength.atEnd());
      int numActivePixels;
      bottomRunLength.copyPixels(
          bottomImage->getRegionEnd() - topImage->getRegionEnd(),
          *outImage->runLengths,
          numActivePixels);
      std::copy(bottomColorBuffer,
                bottomColorBuffer + (numActivePixels * ColorVecSize),
                outColorBuffer);

      assert(bottomRunLength.atEnd());
    }

    outImage->shrinkArrays();

    return outImageHolder;
  }

  bool blendIsOrderDependent() const final { return true; }

  std::unique_ptr<Image> copySubrange(int subregionBegin,
                                      int subregionEnd) const final {
    std::unique_ptr<Image> outImageHolder =
        this->createNew(subregionBegin + this->getRegionBegin(),
                        subregionEnd + this->getRegionBegin());
    ThisType* subImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((subImage != NULL) && "Internal error: createNew bad type.");

    subImage->runLengths->resize(0);
    int activeSubregionBegin;
    int activeSubregionEnd;
    this->copyRunlengthRegion(subregionBegin,
                              subregionEnd,
                              *subImage->runLengths,
                              activeSubregionBegin,
                              activeSubregionEnd);

    // Copy the active pixel data
    Image* copiedActivePixels =
        this->pixelStorage
            ->copySubrange(activeSubregionBegin, activeSubregionEnd)
            .release();
    subImage->pixelStorage = std::shared_ptr<StorageType>(
        dynamic_cast<StorageType*>(copiedActivePixels));

    subImage->shrinkArrays();
    return outImageHolder;
  }

  std::unique_ptr<const Image> window(int subregionBegin,
                                      int subregionEnd) const final {
    std::unique_ptr<Image> outImageHolder =
        this->createNew(subregionBegin + this->getRegionBegin(),
                        subregionEnd + this->getRegionBegin());
    ThisType* subImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((subImage != NULL) && "Internal error: createNew bad type.");

    subImage->runLengths->resize(0);
    int activeSubregionBegin;
    int activeSubregionEnd;
    this->copyRunlengthRegion(subregionBegin,
                              subregionEnd,
                              *subImage->runLengths,
                              activeSubregionBegin,
                              activeSubregionEnd);

    // Copy the active pixel data
    const Image* windowedActivePixels =
        this->pixelStorage->window(activeSubregionBegin, activeSubregionEnd)
            .release();
    subImage->pixelStorage =
        std::shared_ptr<StorageType>(const_cast<StorageType*>(
            dynamic_cast<const StorageType*>(windowedActivePixels)));

    subImage->shrinkArrays();
    return std::unique_ptr<const Image>(outImageHolder.release());
  }

  std::unique_ptr<ImageFull> uncompress() const final {
    std::unique_ptr<Image> outImageTmp =
        this->pixelStorage->createNew(this->getWidth(),
                                      this->getHeight(),
                                      this->getRegionBegin(),
                                      this->getRegionEnd(),
                                      this->getValidViewport());
    std::unique_ptr<StorageType> outImage(
        dynamic_cast<StorageType*>(outImageTmp.release()));
    assert(outImage && "Internal error: Storage type not as expected.");

    int inBufferIndex = 0;
    int outPixelIndex = 0;
    for (auto&& runLength : *this->runLengths) {
      for (int i = 0; i < runLength.backgroundPixels; ++i) {
        std::copy(this->background.color,
                  this->background.color + ColorVecSize,
                  outImage->getColorBuffer(outPixelIndex + i));
      }
      outPixelIndex += runLength.backgroundPixels;

      if (runLength.foregroundPixels > 0) {
        std::copy(this->pixelStorage->getColorBuffer(inBufferIndex),
                  this->pixelStorage->getColorBuffer(
                      inBufferIndex + runLength.foregroundPixels),
                  outImage->getColorBuffer(outPixelIndex));
        inBufferIndex += runLength.foregroundPixels;
        outPixelIndex += runLength.foregroundPixels;
      }
    }

    assert(inBufferIndex == this->pixelStorage->getNumberOfPixels());
    assert(outPixelIndex == outImage->getNumberOfPixels());

    return std::unique_ptr<ImageFull>(outImage.release());
  }


  std::vector<MPI_Request> ISend(int destRank,
                                 MPI_Comm communicator) const final {
    std::vector<MPI_Request> requests =
        this->ISendMetaData(destRank, communicator);

    MPI_Request backgroundRequest;
    MPI_Isend(&this->background,
              sizeof(ThisType::BackgroundInfo),
              MPI_BYTE,
              destRank,
              BACKGROUND_TAG,
              communicator,
              &backgroundRequest);
    requests.push_back(backgroundRequest);

    // Make sure we don't send arrays larger than necessary.
    this->shrinkArrays();

    MPI_Request runLengthsRequest;
    MPI_Isend(this->runLengths->data(),
              sizeof(RunLengthRegion) * this->runLengths->size(),
              MPI_BYTE,
              destRank,
              RUN_LENGTHS_TAG,
              communicator,
              &runLengthsRequest);
    requests.push_back(runLengthsRequest);

    std::vector<MPI_Request> pixelStorageRequests =
        this->pixelStorage->ISend(destRank, communicator);
    requests.insert(requests.end(),
                    pixelStorageRequests.begin(),
                    pixelStorageRequests.end());

    return requests;
  }

  std::vector<MPI_Request> IReceive(int sourceRank,
                                    MPI_Comm communicator) final {
    std::vector<MPI_Request> requests =
        this->IReceiveMetaData(sourceRank, communicator);

    MPI_Request backgroundRequest;
    MPI_Irecv(&this->background,
              sizeof(ThisType::BackgroundInfo),
              MPI_BYTE,
              sourceRank,
              BACKGROUND_TAG,
              communicator,
              &backgroundRequest);
    requests.push_back(backgroundRequest);

    // Make sure run length buffer large enough for maximum size image.
    this->runLengths->resize(this->getNumberOfPixels() / 2 + 1);
    // Also make sure runLengths array is zeroed out so we don't count garbage
    // as pixels.
    std::fill(
        this->runLengths->begin(), this->runLengths->end(), RunLengthRegion());
    MPI_Request runLengthsRequest;
    MPI_Irecv(this->runLengths->data(),
              sizeof(RunLengthRegion) * this->runLengths->size(),
              MPI_BYTE,
              sourceRank,
              RUN_LENGTHS_TAG,
              communicator,
              &runLengthsRequest);
    requests.push_back(runLengthsRequest);

    // Make sure pixel buffer large enough for maximum size image.
    this->pixelStorage->resizeBuffers(0, this->getNumberOfPixels());
    std::vector<MPI_Request> pixelStorageRequests =
        this->pixelStorage->IReceive(sourceRank, communicator);
    requests.insert(requests.end(),
                    pixelStorageRequests.begin(),
                    pixelStorageRequests.end());

    return requests;
  }

 protected:
  void clearImpl(const Color& color, float) final {
    this->setBackground(color);
    this->clearKnownBackground();
  }

  std::unique_ptr<Image> createNewImpl(int _width,
                                       int _height,
                                       int _regionBegin,
                                       int _regionEnd) const final {
    std::unique_ptr<Image> pixelStorageCopy =
        this->pixelStorage->createNew(0, 0);
    StorageType* pixelStorageCopyCast =
        dynamic_cast<StorageType*>(pixelStorageCopy.release());
    assert(pixelStorageCopyCast != nullptr);
    std::shared_ptr<StorageType> pixelStorageCopyShared(pixelStorageCopyCast);
    std::shared_ptr<std::vector<RunLengthRegion>> newRunLengths(
        new std::vector<RunLengthRegion>);
    ThisType* newImage = new ThisType(_width,
                                      _height,
                                      _regionBegin,
                                      _regionEnd,
                                      Viewport(0, 0, _width - 1, _height - 1),
                                      pixelStorageCopyShared,
                                      newRunLengths,
                                      this->background);
    newImage->clearKnownBackground();
    return std::unique_ptr<Image>(newImage);
  }

  std::unique_ptr<const Image> shallowCopyImpl() const final {
    return std::unique_ptr<const Image>(new ThisType(*this));
  }
};

#endif  // IMAGESPARSECOLORONLY_HPP

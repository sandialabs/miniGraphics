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
  std::shared_ptr<std::vector<RunLengthRegion>> runLengths;

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
      std::shared_ptr<StorageType> _pixelStorage,
      std::shared_ptr<std::vector<RunLengthRegion>> _runLengths,
      const BackgroundInfo& _background)
      : ImageSparse(_width, _height, _regionBegin, _regionEnd),
        pixelStorage(_pixelStorage),
        runLengths(_runLengths),
        background(_background) {}

 public:
  ImageSparseColorOnly(const StorageType& toCompress)
      : ImageSparse(toCompress.getWidth(),
                    toCompress.getHeight(),
                    toCompress.getRegionBegin(),
                    toCompress.getRegionEnd()),
        runLengths(new std::vector<RunLengthRegion>) {
    std::unique_ptr<Image> imageBuffer =
        toCompress.createNew(this->getWidth(), this->getHeight(), 0, 0);
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
    int numPixels = toCompress.getNumberOfPixels();
    int numActivePixels = 0;
    int iPixel = 0;
    this->runLengths->resize(0);
    while (iPixel < numPixels) {
      RunLengthRegion runLength;
      while ((iPixel < numPixels) &&
             this->isBackground(toCompress.getColorBuffer(iPixel))) {
        ++runLength.backgroundPixels;
        ++iPixel;
      }
      while ((iPixel < numPixels) &&
             !this->isBackground(toCompress.getColorBuffer(iPixel))) {
        ++runLength.foregroundPixels;
        ++iPixel;
      }
      numActivePixels += runLength.foregroundPixels;
      this->runLengths->push_back(runLength);
    }

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
  void shrinkArrays() const {
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
    assert(totalActivePixels <= this->pixelStorage->getNumberOfPixels());
    this->pixelStorage->resizeBuffers(0, totalActivePixels);
    this->runLengths->resize(runLengthSize);
  }

 public:
  std::unique_ptr<Image> blend(const Image& _otherImage) const final {
    const ThisType* otherImage = dynamic_cast<const ThisType*>(&_otherImage);
    assert((otherImage != NULL) && "Attemptying to blend invalid images.");

    int maxNumActivePixels =
        std::min(this->pixelStorage->getNumberOfPixels() +
                     otherImage->pixelStorage->getNumberOfPixels(),
                 this->getNumberOfPixels());

    const ColorType* topColorBuffer = this->pixelStorage->getColorBuffer();
    const ColorType* bottomColorBuffer =
        otherImage->pixelStorage->getColorBuffer();

    std::unique_ptr<Image> outImageHolder = this->createNew();
    ThisType* outImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((outImage != NULL) && "Internal error: createNew bad type.");
    outImage->pixelStorage->resizeBuffers(0, maxNumActivePixels);
    outImage->runLengths->resize(0);
    ColorType* outColorBuffer = outImage->pixelStorage->getColorBuffer();

    int topRunLengthIndex = 0;
    RunLengthRegion topRunLength;
    int bottomRunLengthIndex = 0;
    RunLengthRegion bottomRunLength;
    outImage->runLengths->push_back(RunLengthRegion());
    while (((topRunLengthIndex < this->runLengths->size()) ||
            (topRunLength.backgroundPixels > 0) ||
            (topRunLength.foregroundPixels > 0)) &&
           ((bottomRunLengthIndex < otherImage->runLengths->size()) ||
            (bottomRunLength.backgroundPixels > 0) ||
            (bottomRunLength.foregroundPixels > 0))) {
      if ((topRunLength.backgroundPixels < 1) &&
          (topRunLength.foregroundPixels < 1)) {
        topRunLength = this->runLengths->at(topRunLengthIndex);
        ++topRunLengthIndex;
      }
      if ((bottomRunLength.backgroundPixels < 1) &&
          (bottomRunLength.foregroundPixels < 1)) {
        bottomRunLength = otherImage->runLengths->at(bottomRunLengthIndex);
        ++bottomRunLengthIndex;
      }

      if ((topRunLength.backgroundPixels > 0) &&
          (bottomRunLength.backgroundPixels > 0)) {
        // Case 1: Both images are in background. Just add to inactive count.
        if (outImage->runLengths->back().foregroundPixels != 0) {
          outImage->runLengths->push_back(RunLengthRegion());
        }
        int numInactive = std::min(topRunLength.backgroundPixels,
                                   bottomRunLength.backgroundPixels);
        outImage->runLengths->back().backgroundPixels += numInactive;
        topRunLength.backgroundPixels -= numInactive;
        bottomRunLength.backgroundPixels -= numInactive;
      } else if (topRunLength.backgroundPixels > 0) {
        // Case 2: Top image in background, bottom image in foreground.
        int numPixels = std::min(topRunLength.backgroundPixels,
                                 bottomRunLength.foregroundPixels);

        std::copy(bottomColorBuffer,
                  bottomColorBuffer + (numPixels * ColorVecSize),
                  outColorBuffer);
        bottomColorBuffer += numPixels * ColorVecSize;
        outColorBuffer += numPixels * ColorVecSize;

        topRunLength.backgroundPixels -= numPixels;
        bottomRunLength.foregroundPixels -= numPixels;
        outImage->runLengths->back().foregroundPixels += numPixels;
      } else if (bottomRunLength.backgroundPixels > 0) {
        // Case 3: Bottom image in background, top image in foreground.
        int numPixels = std::min(bottomRunLength.backgroundPixels,
                                 topRunLength.foregroundPixels);

        std::copy(topColorBuffer,
                  topColorBuffer + (numPixels * ColorVecSize),
                  outColorBuffer);
        topColorBuffer += numPixels * ColorVecSize;
        outColorBuffer += numPixels * ColorVecSize;

        bottomRunLength.backgroundPixels -= numPixels;
        topRunLength.foregroundPixels -= numPixels;
        outImage->runLengths->back().foregroundPixels += numPixels;
      } else {
        // Case 4: Both images are in foreground, blend them.
        int numPixels = std::min(topRunLength.foregroundPixels,
                                 bottomRunLength.foregroundPixels);

        for (int pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex) {
          Features::blend(topColorBuffer + (pixelIndex * ColorVecSize),
                          bottomColorBuffer + (pixelIndex * ColorVecSize),
                          outColorBuffer + (pixelIndex * ColorVecSize));
        }

        topColorBuffer += numPixels * ColorVecSize;
        bottomColorBuffer += numPixels * ColorVecSize;
        outColorBuffer += numPixels * ColorVecSize;

        topRunLength.foregroundPixels -= numPixels;
        bottomRunLength.foregroundPixels -= numPixels;
        outImage->runLengths->back().foregroundPixels += numPixels;
      }
    }

    outImage->shrinkArrays();

    return outImageHolder;
  }

  bool blendIsOrderDependent() const final { return true; }

  std::unique_ptr<Image> copySubrange(int subregionBegin,
                                      int subregionEnd) const final {
    assert(subregionBegin <= subregionEnd);
    assert(subregionBegin >= 0);
    assert(subregionEnd <= this->getNumberOfPixels());

    std::unique_ptr<Image> outImageHolder =
        this->createNew(this->getWidth(),
                        this->getHeight(),
                        subregionBegin + this->getRegionBegin(),
                        subregionEnd + this->getRegionBegin());
    ThisType* subImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((subImage != NULL) && "Internal error: createNew bad type.");

    subImage->pixelStorage->resizeBuffers(
        0,
        std::min(this->getNumberOfPixels(),
                 this->pixelStorage->getNumberOfPixels()));
    subImage->runLengths->resize(0);

    // Skip over run lengths before subregionBegin
    int pixelIndex = 0;
    int runLengthIndex = 0;
    int inBufferIndex = 0;
    int outBufferIndex = 0;
    while (pixelIndex < subregionBegin) {
      const RunLengthRegion& runLength = this->runLengths->at(runLengthIndex);
      if ((pixelIndex + runLength.backgroundPixels +
           runLength.foregroundPixels) > subregionBegin) {
        break;
      }
      pixelIndex += runLength.backgroundPixels + runLength.foregroundPixels;
      inBufferIndex += runLength.foregroundPixels;
      ++runLengthIndex;
    }

    while (pixelIndex < subregionEnd) {
      RunLengthRegion runLength = this->runLengths->at(runLengthIndex);
      ++runLengthIndex;

      // Handle special case where pixelIndex is before subregionBegin (which
      // can happen in the first run length).
      if (pixelIndex < subregionBegin) {
        int pixelsToSkip = subregionBegin - pixelIndex;
        if (pixelsToSkip <= runLength.backgroundPixels) {
          runLength.backgroundPixels -= pixelsToSkip;
        } else {
          pixelsToSkip -= runLength.backgroundPixels;
          runLength.backgroundPixels = 0;
          runLength.foregroundPixels -= pixelsToSkip;
          inBufferIndex += pixelsToSkip;
        }
        pixelIndex = subregionBegin;
      }

      // Handle special case where regionEnd is in the middle of the run length
      int pixelsRemaining = subregionEnd - pixelIndex;
      if (pixelsRemaining < runLength.backgroundPixels) {
        runLength.backgroundPixels = pixelsRemaining;
        runLength.foregroundPixels = 0;
      } else if (pixelsRemaining <
                 (runLength.backgroundPixels + runLength.foregroundPixels)) {
        runLength.foregroundPixels =
            pixelsRemaining - runLength.backgroundPixels;
      }

      // Copy the pixel information.
      std::copy(this->pixelStorage->getColorBuffer(inBufferIndex),
                this->pixelStorage->getColorBuffer(inBufferIndex +
                                                   runLength.foregroundPixels),
                subImage->pixelStorage->getColorBuffer(outBufferIndex));
      subImage->runLengths->push_back(runLength);

      // Move indices to end of recorded pixels.
      pixelIndex += runLength.backgroundPixels + runLength.foregroundPixels;
      inBufferIndex += runLength.foregroundPixels;
      outBufferIndex += runLength.foregroundPixels;
    }

    subImage->shrinkArrays();
    return outImageHolder;
  }

  std::unique_ptr<ImageFull> uncompress() const final {
    std::unique_ptr<Image> outImageTmp =
        this->pixelStorage->createNew(this->getWidth(),
                                      this->getHeight(),
                                      this->getRegionBegin(),
                                      this->getRegionEnd());
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

    const void* rlBuffer;
    int dummyBuffer;

    if (this->runLengths->size() != 0) {
      rlBuffer = &this->runLengths->front();
    } else {
      // If our run length vector is empty we still need to send a message, and
      // I suspect some implementations of MPI will still want a valid buffer
      // even if we are not actually using it. So in this case, just set up a
      // dummy buffer.
      rlBuffer = &dummyBuffer;
    }

    MPI_Request runLengthsRequest;
    MPI_Isend(rlBuffer,
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
    MPI_Request runLengthsRequest;
    MPI_Irecv(&this->runLengths->front(),
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
        this->pixelStorage->createNew(_width, _height, 0, 0);
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

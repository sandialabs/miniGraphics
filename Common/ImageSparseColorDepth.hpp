// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGESPARSECOLORDEPTH_HPP
#define IMAGESPARSECOLORDEPTH_HPP

#include "ImageSparse.hpp"

#include "ImageColorDepth.hpp"

#include <algorithm>

template <typename Features>
class ImageSparseColorDepth : public ImageSparse {
 public:
  using ColorType = typename Features::ColorType;
  using DepthType = typename Features::DepthType;
  static constexpr int ColorVecSize = Features::ColorVecSize;

 private:
  using ThisType = ImageSparseColorDepth<Features>;
  using StorageType = ImageColorDepth<Features>;

  std::shared_ptr<StorageType> pixelStorage;

  static constexpr int BACKGROUND_TAG = 89016;
  static constexpr int RUN_LENGTHS_TAG = 89017;

  struct BackgroundInfo {
    ColorType color[ColorVecSize];
    DepthType depth;
  };
  BackgroundInfo background;
  void setBackground(const Color& color, float depth) {
    Features::encodeColor(color, this->background.color);
    Features::encodeDepth(depth, &this->background.depth);
  }

  ImageSparseColorDepth(
      int _width,
      int _height,
      int _regionBegin,
      int _regionEnd,
      std::shared_ptr<StorageType> _pixelStorage,
      std::shared_ptr<std::vector<RunLengthRegion>> _runLengths,
      const BackgroundInfo& _background)
      : ImageSparse(_width, _height, _regionBegin, _regionEnd, _runLengths),
        pixelStorage(_pixelStorage),
        background(_background) {}

 public:
  ImageSparseColorDepth(const StorageType& toCompress)
      : ImageSparse(toCompress.getWidth(),
                    toCompress.getHeight(),
                    toCompress.getRegionBegin(),
                    toCompress.getRegionEnd()) {
    std::unique_ptr<Image> imageBuffer =
        toCompress.createNew(this->getWidth(), this->getHeight(), 0, 0);
    StorageType* pixelStorageP = dynamic_cast<StorageType*>(imageBuffer.get());
    if (pixelStorageP != nullptr) {
      imageBuffer.release();
      this->pixelStorage.reset(pixelStorageP);
    } else {
      throw std::runtime_error(
          "ImageSparseColorDepth called with bad image type.");
    }
    this->setBackground(Color(0, 0, 0, 0), 1.0f);
    this->compress(toCompress);
  }

 private:
  bool isBackground(const DepthType& depth) const {
    return !Features::closer(depth, this->background.depth);
  }

  void compress(const StorageType& toCompress) {
    int numPixels = toCompress.getNumberOfPixels();
    int numActivePixels = 0;
    int iPixel = 0;
    this->runLengths->resize(0);
    while (iPixel < numPixels) {
      RunLengthRegion runLength;
      while ((iPixel < numPixels) &&
             this->isBackground(*toCompress.getDepthBuffer(iPixel))) {
        ++runLength.backgroundPixels;
        ++iPixel;
      }
      while ((iPixel < numPixels) &&
             !this->isBackground(*toCompress.getDepthBuffer(iPixel))) {
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
        std::copy(
            toCompress.getDepthBuffer(iPixel),
            toCompress.getDepthBuffer(iPixel + runLength.foregroundPixels),
            this->pixelStorage->getDepthBuffer(iActivePixel));
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

    if (topImage->pixelStorage->getNumberOfPixels() < 1) {
      return bottomImage->deepCopy();
    }
    if (bottomImage->pixelStorage->getNumberOfPixels() < 1) {
      return topImage->deepCopy();
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
    const DepthType* topDepthBuffer = topImage->pixelStorage->getDepthBuffer();
    const ColorType* bottomColorBuffer =
        bottomImage->pixelStorage->getColorBuffer();
    const DepthType* bottomDepthBuffer =
        bottomImage->pixelStorage->getDepthBuffer();

    std::unique_ptr<Image> outImageHolder = this->createNew(
        this->getWidth(), this->getHeight(), totalRegionBegin, totalRegionEnd);
    ThisType* outImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((outImage != NULL) && "Internal error: createNew bad type.");
    outImage->pixelStorage->resizeBuffers(0, maxNumActivePixels);
    outImage->runLengths->resize(0);
    ColorType* outColorBuffer = outImage->pixelStorage->getColorBuffer();
    DepthType* outDepthBuffer = outImage->pixelStorage->getDepthBuffer();

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
      std::copy(
          topDepthBuffer, topDepthBuffer + numActivePixels, outDepthBuffer);
      topDepthBuffer += numActivePixels;
      outDepthBuffer += numActivePixels;
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
      std::copy(bottomDepthBuffer,
                bottomDepthBuffer + numActivePixels,
                outDepthBuffer);
      bottomDepthBuffer += numActivePixels;
      outDepthBuffer += numActivePixels;
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

        std::copy(
            bottomDepthBuffer, bottomDepthBuffer + numPixels, outDepthBuffer);
        bottomDepthBuffer += numPixels;
        outDepthBuffer += numPixels;

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

        std::copy(topDepthBuffer, topDepthBuffer + numPixels, outDepthBuffer);
        topDepthBuffer += numPixels;
        outDepthBuffer += numPixels;

        bottomRunLength.advance(numPixels);
        topRunLength.advance(numPixels);
        outImage->runLengths->back().foregroundPixels += numPixels;
      } else {
        // Case 4: Both images are in foreground, blend them.
        int numPixels = std::min(topRunLength.getWorkingForeground(),
                                 bottomRunLength.getWorkingForeground());

        for (int pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex) {
          if (Features::closer(bottomDepthBuffer[pixelIndex],
                               topDepthBuffer[pixelIndex])) {
            std::copy(bottomColorBuffer + pixelIndex * ColorVecSize,
                      bottomColorBuffer + (pixelIndex + 1) * ColorVecSize,
                      outColorBuffer + pixelIndex * ColorVecSize);
            outDepthBuffer[pixelIndex] = bottomDepthBuffer[pixelIndex];
          } else {
            std::copy(topColorBuffer + pixelIndex * ColorVecSize,
                      topColorBuffer + (pixelIndex + 1) * ColorVecSize,
                      outColorBuffer + pixelIndex * ColorVecSize);
            outDepthBuffer[pixelIndex] = topDepthBuffer[pixelIndex];
          }
        }

        topColorBuffer += numPixels * ColorVecSize;
        bottomColorBuffer += numPixels * ColorVecSize;
        outColorBuffer += numPixels * ColorVecSize;

        topDepthBuffer += numPixels;
        bottomDepthBuffer += numPixels;
        outDepthBuffer += numPixels;

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
      std::copy(
          topDepthBuffer, topDepthBuffer + numActivePixels, outDepthBuffer);

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
      std::copy(bottomDepthBuffer,
                bottomDepthBuffer + numActivePixels,
                outDepthBuffer);

      assert(bottomRunLength.atEnd());
    }

    outImage->shrinkArrays();

    return outImageHolder;
  }

  bool blendIsOrderDependent() const final { return false; }

  std::unique_ptr<Image> copySubrange(int subregionBegin,
                                      int subregionEnd) const final {
    std::unique_ptr<Image> outImageHolder =
        this->createNew(this->getWidth(),
                        this->getHeight(),
                        subregionBegin + this->getRegionBegin(),
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
        this->createNew(this->getWidth(),
                        this->getHeight(),
                        subregionBegin + this->getRegionBegin(),
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
        *outImage->getDepthBuffer(outPixelIndex + i) = this->background.depth;
      }
      outPixelIndex += runLength.backgroundPixels;

      if (runLength.foregroundPixels > 0) {
        std::copy(this->pixelStorage->getColorBuffer(inBufferIndex),
                  this->pixelStorage->getColorBuffer(
                      inBufferIndex + runLength.foregroundPixels),
                  outImage->getColorBuffer(outPixelIndex));
        std::copy(this->pixelStorage->getDepthBuffer(inBufferIndex),
                  this->pixelStorage->getDepthBuffer(
                      inBufferIndex + runLength.foregroundPixels),
                  outImage->getDepthBuffer(outPixelIndex));
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
  void clearImpl(const Color& color, float depth) final {
    this->setBackground(color, depth);
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

#endif  // IMAGESPARSECOLORDEPTH_HPP

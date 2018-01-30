// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGECOLORDEPTH_HPP
#define IMAGECOLORDEPTH_HPP

#include "Image.hpp"

#include <memory>
#include <vector>

template <typename _ColorType, int _ColorVecSize, typename _DepthType>
class ImageColorDepth : public Image {
 public:
  using ColorType = _ColorType;
  using DepthType = _DepthType;
  static const int ColorVecSize = _ColorVecSize;  // Should be constexpr

 private:
  using ThisType = ImageColorDepth<ColorType, ColorVecSize, DepthType>;

  std::shared_ptr<std::vector<ColorType>> colorBuffer;
  std::shared_ptr<std::vector<DepthType>> depthBuffer;

  static const int COLOR_BUFFER_TAG = 12900;  // Should be constexpr
  static const int DEPTH_BUFFER_TAG = 12901;

 public:
  ImageColorDepth(int _width, int _height)
      : Image(_width, _height),
        colorBuffer(
            new std::vector<ColorType>(_width * _height * ColorVecSize)),
        depthBuffer(new std::vector<DepthType>(_width * _height)) {}

  ImageColorDepth(int _width, int _height, int _regionBegin, int _regionEnd)
      : Image(_width, _height, _regionBegin, _regionEnd),
        colorBuffer(new std::vector<ColorType>((_regionEnd - _regionBegin) *
                                               ColorVecSize)),
        depthBuffer(new std::vector<DepthType>(_regionEnd - _regionBegin)) {}

  ~ImageColorDepth() = default;

  ColorType* getColorBuffer(int pixelIndex = 0) {
    return &this->colorBuffer->front() + (pixelIndex * ColorVecSize);
  }
  const ColorType* getColorBuffer(int pixelIndex = 0) const {
    return &this->colorBuffer->front() + (pixelIndex * ColorVecSize);
  }

  DepthType* getDepthBuffer(int pixelIndex = 0) {
    return &this->depthBuffer->front() + pixelIndex;
  }
  const DepthType* getDepthBuffer(int pixelIndex = 0) const {
    return &this->depthBuffer->front() + pixelIndex;
  }

  void clear(const Color& color = Color(0, 0, 0, 0), float depth = 1.0f) final {
    int numPixels = this->getNumberOfPixels();
    if (numPixels < 1) {
      return;
    }

    // Encode the color and depth by calling setColor/setDepth for first pixel.
    this->setColor(0, color);
    this->setDepth(0, depth);

    ColorType* cBuffer = this->getColorBuffer();
    DepthType* dBuffer = this->getDepthBuffer();

    ColorType colorValue[ColorVecSize];
    for (int colorComponent = 0; colorComponent < ColorVecSize;
         ++colorComponent) {
      colorValue[colorComponent] = cBuffer[colorComponent];
    }
    DepthType depthValue = dBuffer[0];

    for (int pixelIndex = 1; pixelIndex < numPixels; ++pixelIndex) {
      for (int colorComponent = 0; colorComponent < ColorVecSize;
           ++colorComponent) {
        cBuffer[pixelIndex * ColorVecSize + colorComponent] =
            colorValue[colorComponent];
      }
      dBuffer[pixelIndex] = depthValue;
    }
  }

  std::unique_ptr<Image> blend(const Image* _otherImage) const final {
    const ThisType* otherImage = dynamic_cast<const ThisType*>(_otherImage);
    assert((otherImage != NULL) && "Attempting to blend invalid images.");

    int numPixels = this->getNumberOfPixels();
    assert((numPixels == otherImage->getNumberOfPixels()) &&
           "Attempting to blend images of different sizes.");

    const ColorType* topColorBuffer = this->getColorBuffer();
    const DepthType* topDepthBuffer = this->getDepthBuffer();
    const ColorType* bottomColorBuffer = otherImage->getColorBuffer();
    const DepthType* bottomDepthBuffer = otherImage->getDepthBuffer();

    std::unique_ptr<Image> outImageHolder =
        this->createNew(this->getWidth(),
                        this->getHeight(),
                        this->getRegionBegin(),
                        this->getRegionEnd());
    ThisType* outImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((outImage != NULL) && "Internal error: createNew bad type.");
    ColorType* outColorBuffer = outImage->getColorBuffer();
    DepthType* outDepthBuffer = outImage->getDepthBuffer();

    for (int pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex) {
      if (bottomDepthBuffer[pixelIndex] < topDepthBuffer[pixelIndex]) {
        for (int colorComponent = 0; colorComponent < ColorVecSize;
             ++colorComponent) {
          outColorBuffer[pixelIndex * ColorVecSize + colorComponent] =
              bottomColorBuffer[pixelIndex * ColorVecSize + colorComponent];
        }
        outDepthBuffer[pixelIndex] = bottomDepthBuffer[pixelIndex];
      } else {
        for (int colorComponent = 0; colorComponent < ColorVecSize;
             ++colorComponent) {
          outColorBuffer[pixelIndex * ColorVecSize + colorComponent] =
              topColorBuffer[pixelIndex * ColorVecSize + colorComponent];
        }
        outDepthBuffer[pixelIndex] = topDepthBuffer[pixelIndex];
      }
    }

    return outImageHolder;
  }

  bool blendIsOrderDependent() const final { return false; }

  std::unique_ptr<Image> copySubrange(int subregionBegin,
                                      int subregionEnd) const final {
    assert(subregionBegin <= subregionEnd);

    std::unique_ptr<Image> outImageHolder =
        this->createNew(this->getWidth(),
                        this->getHeight(),
                        subregionBegin + this->getRegionBegin(),
                        subregionEnd + this->getRegionBegin());
    ThisType* subImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((subImage != NULL) && "Internal error: createNew bad type.");

    std::copy(this->getColorBuffer(subregionBegin),
              this->getColorBuffer(subregionEnd),
              subImage->getColorBuffer());
    std::copy(this->getDepthBuffer(subregionBegin),
              this->getDepthBuffer(subregionEnd),
              subImage->getDepthBuffer());

    return outImageHolder;
  }

  std::unique_ptr<Image> Gather(int recvRank,
                                MPI_Comm communicator) const final {
    int rank;
    MPI_Comm_rank(communicator, &rank);

    int numProc;
    MPI_Comm_size(communicator, &numProc);

    std::vector<int> allRegionBegin(numProc);
    int regionBegin = this->getRegionBegin() * sizeof(ColorType) * ColorVecSize;
    MPI_Gather(&regionBegin,
               1,
               MPI_INT,
               &allRegionBegin.front(),
               1,
               MPI_INT,
               recvRank,
               communicator);

    std::vector<int> allRegionCounts(numProc);
    int dataSize = this->getNumberOfPixels() * sizeof(ColorType) * ColorVecSize;
    MPI_Gather(&dataSize,
               1,
               MPI_INT,
               &allRegionCounts.front(),
               1,
               MPI_INT,
               recvRank,
               communicator);

    std::unique_ptr<Image> outImageHolder =
        this->createNew(this->getWidth(),
                        this->getHeight(),
                        0,
                        this->getWidth() * this->getHeight());
    ThisType* recvImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((recvImage != NULL) && "Internal error: createNew bad type.");

    MPI_Gatherv(this->getColorBuffer(),
                dataSize,
                MPI_BYTE,
                recvImage->getColorBuffer(),
                &allRegionCounts.front(),
                &allRegionBegin.front(),
                MPI_BYTE,
                recvRank,
                communicator);

    return outImageHolder;
  }

  std::vector<MPI_Request> ISend(int destRank,
                                 MPI_Comm communicator) const final {
    std::vector<MPI_Request> requests =
        this->ISendMetaData(destRank, communicator);

    MPI_Request colorRequest;
    MPI_Isend(&this->colorBuffer->front(),
              this->getNumberOfPixels() * sizeof(ColorType) * ColorVecSize,
              MPI_BYTE,
              destRank,
              COLOR_BUFFER_TAG,
              communicator,
              &colorRequest);
    requests.push_back(colorRequest);

    MPI_Request depthRequest;
    MPI_Isend(&this->depthBuffer->front(),
              this->getNumberOfPixels() * sizeof(DepthType),
              MPI_BYTE,
              destRank,
              DEPTH_BUFFER_TAG,
              communicator,
              &depthRequest);
    requests.push_back(depthRequest);

    return requests;
  }

  std::vector<MPI_Request> IReceive(int sourceRank,
                                    MPI_Comm communicator) final {
    std::vector<MPI_Request> requests =
        this->IReceiveMetaData(sourceRank, communicator);

    MPI_Request colorRequest;
    MPI_Irecv(&this->colorBuffer->front(),
              this->getNumberOfPixels() * sizeof(ColorType) * ColorVecSize,
              MPI_BYTE,
              sourceRank,
              COLOR_BUFFER_TAG,
              communicator,
              &colorRequest);
    requests.push_back(colorRequest);

    MPI_Request depthRequest;
    MPI_Irecv(&this->depthBuffer->front(),
              this->getNumberOfPixels() * sizeof(DepthType),
              MPI_BYTE,
              sourceRank,
              DEPTH_BUFFER_TAG,
              communicator,
              &depthRequest);
    requests.push_back(depthRequest);

    return requests;
  }
};

#endif  // IMAGECOLORDEPTH_HPP

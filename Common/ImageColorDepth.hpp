// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGECOLORDEPTH_HPP
#define IMAGECOLORDEPTH_HPP

#include "ImageFull.hpp"

#include <memory>
#include <vector>

struct ImageColorDepthBase {};

/// \brief Implementation of color/depth images
///
/// ImageColorDepth is a base class of images that have both color and depth
/// buffers and use a depth comparison for blending. When subclassing this
/// class, the derived class is expected to provide a "features" structure as
/// the template argument. This features structure must contain the following
/// items.
///   - A ColorType typename specifying the base type of a color component.
///   - A ColorVecSize constant expression of the number of color components.
///   - A DepthType typename specifying the base type of a depth component.
///   - A static function named closer that takes two depth values (of
///     DepthType) and returns true if the first value is closer than the
///     second.
///   - A static function named encodeColor that takes a Color object and
///     fills a given array of ColorType values.
///   - A static function named decodeColor that takes an array of ColorType
///     values and returns a Color object.
///   - A static function named encodeDepth that takes a float value and
///     fills a given array (of size one) of DepthType.
///   - A static function named decodeDepth that takes an array (of size 1)
///     of DepthType and returns a float object.
///
template <typename Features>
class ImageColorDepth : public ImageFull, ImageColorDepthBase {
 public:
  using ColorType = typename Features::ColorType;
  using DepthType = typename Features::DepthType;
  static constexpr int ColorVecSize = Features::ColorVecSize;

 private:
  using ThisType = ImageColorDepth<Features>;

  std::shared_ptr<std::vector<ColorType>> colorBuffer;
  std::shared_ptr<std::vector<DepthType>> depthBuffer;

  static constexpr int COLOR_BUFFER_TAG = 12900;
  static constexpr int DEPTH_BUFFER_TAG = 12901;

 public:
  ImageColorDepth(int _width, int _height)
      : ImageFull(_width, _height),
        colorBuffer(
            new std::vector<ColorType>(_width * _height * ColorVecSize)),
        depthBuffer(new std::vector<DepthType>(_width * _height)) {}

  ImageColorDepth(int _width, int _height, int _regionBegin, int _regionEnd)
      : ImageFull(_width, _height, _regionBegin, _regionEnd),
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

  void resizeBuffers(int newRegionBegin, int newRegionEnd) {
    this->resize(
        this->getWidth(), this->getHeight(), newRegionBegin, newRegionEnd);
    this->colorBuffer->resize(this->getNumberOfPixels() * ColorVecSize);
    this->depthBuffer->resize(this->getNumberOfPixels());
  }

  Color getColor(int pixelIndex) const final {
    assert(pixelIndex >= 0);
    assert(pixelIndex < this->getNumberOfPixels());

    return Features::decodeColor(this->getColorBuffer(pixelIndex));
  }

  void setColor(int pixelIndex, const Color& color) final {
    assert(pixelIndex >= 0);
    assert(pixelIndex < this->getNumberOfPixels());

    Features::encodeColor(color, this->getColorBuffer(pixelIndex));
  }

  float getDepth(int pixelIndex) const final {
    assert(pixelIndex >= 0);
    assert(pixelIndex < this->getNumberOfPixels());

    return Features::decodeDepth(this->getDepthBuffer(pixelIndex));
  }

  void setDepth(int pixelIndex, float depth) final {
    assert(pixelIndex >= 0);
    assert(pixelIndex < this->getNumberOfPixels());

    Features::encodeDepth(depth, this->getDepthBuffer(pixelIndex));
  }

  std::unique_ptr<Image> blend(const Image& _otherImage) const final {
    const ThisType* otherImage = dynamic_cast<const ThisType*>(&_otherImage);
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
      if (Features::closer(bottomDepthBuffer[pixelIndex],
                           topDepthBuffer[pixelIndex])) {
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

  std::unique_ptr<ImageFull> Gather(int recvRank,
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
    ThisType* recvImage = dynamic_cast<ThisType*>(outImageHolder.release());
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

    return std::unique_ptr<ImageFull>(recvImage);
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

 protected:
  void clearImpl(const Color& color, float depth) final {
    int numPixels = this->getNumberOfPixels();
    if (numPixels < 1) {
      return;
    }

    ColorType colorValue[ColorVecSize];
    Features::encodeColor(color, colorValue);
    DepthType depthValue;
    Features::encodeDepth(depth, &depthValue);

    ColorType* cBuffer = this->getColorBuffer();
    DepthType* dBuffer = this->getDepthBuffer();

    for (int pixelIndex = 1; pixelIndex < numPixels; ++pixelIndex) {
      for (int colorComponent = 0; colorComponent < ColorVecSize;
           ++colorComponent) {
        cBuffer[pixelIndex * ColorVecSize + colorComponent] =
            colorValue[colorComponent];
      }
      dBuffer[pixelIndex] = depthValue;
    }
  }
};

#endif  // IMAGECOLORDEPTH_HPP

// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGECOLORONLY_HPP
#define IMAGECOLORONLY_HPP

#include "ImageFull.hpp"

#include <memory>
#include <vector>

struct ImageColorOnlyBase {};

/// \brief Implementation of color-only images
///
/// ImageColorOnly is a base class of images that have a color buffer and use
/// an order-dependent color alpha blending. When subclassing this class, the
/// derived class is expected to provide a "features" structure as the template
/// argument. This features structure must contain the following items.
///   - A ColorType typename specifying the base type of a color component.
///   - A ColorVecSize constant expression of the number of color components.
///   - A static function named blend that takes takes two ColorType arrays
///     each representing a single color and a third ColorType array to put
///     the result of blending one on top of the other.
///   - A static function named encodeColor that takes a Color object and
///     fills a given array of ColorType values.
///   - A static function named decodeColor that takes an array of ColorType
///     values and returns a Color object.
///
template <typename Features>
class ImageColorOnly : public ImageFull, ImageColorOnlyBase {
 public:
  using ColorType = typename Features::ColorType;
  static constexpr int ColorVecSize = Features::ColorVecSize;

 private:
  using ThisType = ImageColorOnly<Features>;

  std::shared_ptr<std::vector<ColorType>> colorBuffer;

  static const int COLOR_BUFFER_TAG = 12900;  // Should be constexpr

 public:
  ImageColorOnly(int _width, int _height)
      : ImageFull(_width, _height), colorBuffer(new std::vector<ColorType>) {
    this->resizeBuffers(this->getRegionBegin(), this->getRegionEnd());
  }

  ImageColorOnly(int _width, int _height, int _regionBegin, int _regionEnd)
      : ImageFull(_width, _height, _regionBegin, _regionEnd),
        colorBuffer(new std::vector<ColorType>) {
    this->resizeBuffers(this->getRegionBegin(), this->getRegionEnd());
  }

  ~ImageColorOnly() = default;

  ColorType* getColorBuffer(int pixelIndex = 0) {
    if (!this->colorBuffer->empty()) {
      return &this->colorBuffer->front() + (pixelIndex * ColorVecSize);
    } else {
      return nullptr;
    }
  }
  const ColorType* getColorBuffer(int pixelIndex = 0) const {
    if (!this->colorBuffer->empty()) {
      return &this->colorBuffer->front() + (pixelIndex * ColorVecSize);
    } else {
      return nullptr;
    }
  }

  void resizeBuffers(int newRegionBegin, int newRegionEnd) {
    this->resize(
        this->getWidth(), this->getHeight(), newRegionBegin, newRegionEnd);
    this->colorBuffer->resize(this->getNumberOfPixels() * ColorVecSize);
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

  float getDepth(int) const final {
    // No depth
    return 1.0f;
  }

  void setDepth(int, float) final {
    // No depth
  }

  std::unique_ptr<Image> blend(const Image& _otherImage) const final {
    const ThisType* otherImage = dynamic_cast<const ThisType*>(&_otherImage);
    assert((otherImage != NULL) && "Attempting to blend invalid images.");

    int numPixels = this->getNumberOfPixels();
    assert((numPixels == otherImage->getNumberOfPixels()) &&
           "Attempting to blend images of different sizes.");

    const ColorType* topColorBuffer = this->getColorBuffer();
    const ColorType* bottomColorBuffer = otherImage->getColorBuffer();

    std::unique_ptr<Image> outImageHolder =
        this->createNew(this->getWidth(),
                        this->getHeight(),
                        this->getRegionBegin(),
                        this->getRegionEnd());
    ThisType* outImage = dynamic_cast<ThisType*>(outImageHolder.get());
    assert((outImage != NULL) && "Internal error: createNew bad type.");
    ColorType* outColorBuffer = outImage->getColorBuffer();

    for (int pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex) {
      Features::blend(topColorBuffer + (pixelIndex * ColorVecSize),
                      bottomColorBuffer + (pixelIndex * ColorVecSize),
                      outColorBuffer + (pixelIndex * ColorVecSize));
    }

    return outImageHolder;
  }

  bool blendIsOrderDependent() const final { return true; }

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
    MPI_Isend(this->getColorBuffer(),
              this->getNumberOfPixels() * sizeof(ColorType) * ColorVecSize,
              MPI_BYTE,
              destRank,
              COLOR_BUFFER_TAG,
              communicator,
              &colorRequest);
    requests.push_back(colorRequest);

    return requests;
  }

  std::vector<MPI_Request> IReceive(int sourceRank,
                                    MPI_Comm communicator) final {
    std::vector<MPI_Request> requests =
        this->IReceiveMetaData(sourceRank, communicator);

    MPI_Request colorRequest;
    MPI_Irecv(this->getColorBuffer(),
              this->getNumberOfPixels() * sizeof(ColorType) * ColorVecSize,
              MPI_BYTE,
              sourceRank,
              COLOR_BUFFER_TAG,
              communicator,
              &colorRequest);
    requests.push_back(colorRequest);

    return requests;
  }

 protected:
  void clearImpl(const Color& color, float) final {
    int numPixels = this->getNumberOfPixels();
    if (numPixels < 1) {
      return;
    }

    ColorType colorValue[ColorVecSize];
    Features::encodeColor(color, colorValue);

    ColorType* cBuffer = this->getColorBuffer();

    for (int pixelIndex = 1; pixelIndex < numPixels; ++pixelIndex) {
      for (int colorComponent = 0; colorComponent < ColorVecSize;
           ++colorComponent) {
        cBuffer[pixelIndex * ColorVecSize + colorComponent] =
            colorValue[colorComponent];
      }
    }
  }
};

#endif  // IMAGECOLORONLY_HPP

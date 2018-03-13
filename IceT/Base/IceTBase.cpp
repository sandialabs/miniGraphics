// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "IceTBase.hpp"

#include <Common/ImageRGBAFloatColorOnly.hpp>
#include <Common/ImageRGBAUByteColorFloatDepth.hpp>
#include <Common/ImageRGBAUByteColorOnly.hpp>
#include <Common/ImageRGBFloatColorDepth.hpp>
#include <Common/Timer.hpp>

#include <IceTMPI.h>

#include <array>

template <typename ImageType>
struct IceTImageProperties;

template <>
struct IceTImageProperties<ImageRGBAFloatColorOnly> {
  static constexpr IceTEnum colorFormat = ICET_IMAGE_COLOR_RGBA_FLOAT;
  static constexpr IceTEnum depthFormat = ICET_IMAGE_DEPTH_NONE;
  static const IceTVoid* colorBuffer(const ImageRGBAFloatColorOnly& image) {
    return image.getColorBuffer();
  }
  static const IceTVoid* depthBuffer(const ImageRGBAFloatColorOnly&) {
    return nullptr;
  }
  static const IceTFloat* colorBuffer(const IceTImage image) {
    return icetImageGetColorcf(image);
  }
};

template <>
struct IceTImageProperties<ImageRGBAUByteColorFloatDepth> {
  static constexpr IceTEnum colorFormat = ICET_IMAGE_COLOR_RGBA_UBYTE;
  static constexpr IceTEnum depthFormat = ICET_IMAGE_DEPTH_FLOAT;
  static const IceTVoid* colorBuffer(
      const ImageRGBAUByteColorFloatDepth& image) {
    return image.getColorBuffer();
  }
  static const IceTVoid* depthBuffer(
      const ImageRGBAUByteColorFloatDepth& image) {
    return image.getDepthBuffer();
  }
  static const IceTUInt* colorBuffer(const IceTImage image) {
    return icetImageGetColorcui(image);
  }
};

template <>
struct IceTImageProperties<ImageRGBAUByteColorOnly> {
  static constexpr IceTEnum colorFormat = ICET_IMAGE_COLOR_RGBA_UBYTE;
  static constexpr IceTEnum depthFormat = ICET_IMAGE_DEPTH_NONE;
  static const IceTVoid* colorBuffer(const ImageRGBAUByteColorOnly& image) {
    return image.getColorBuffer();
  }
  static const IceTVoid* depthBuffer(const ImageRGBAUByteColorOnly&) {
    return nullptr;
  }
  static const IceTUInt* colorBuffer(const IceTImage image) {
    return icetImageGetColorcui(image);
  }
};

template <>
struct IceTImageProperties<ImageRGBFloatColorDepth> {
  static constexpr IceTEnum colorFormat = ICET_IMAGE_COLOR_RGB_FLOAT;
  static constexpr IceTEnum depthFormat = ICET_IMAGE_DEPTH_FLOAT;
  static const IceTVoid* colorBuffer(const ImageRGBFloatColorDepth& image) {
    return image.getColorBuffer();
  }
  static const IceTVoid* depthBuffer(const ImageRGBFloatColorDepth& image) {
    return image.getDepthBuffer();
  }
  static const IceTFloat* colorBuffer(const IceTImage image) {
    return icetImageGetColorf(image);
  }
};

IceTBase::IceTBase() { this->communicatorCopy = MPI_COMM_NULL; }

IceTBase::~IceTBase() {
  int mpiFinalized;
  MPI_Finalized(&mpiFinalized);
  if (!mpiFinalized && (this->communicatorCopy != MPI_COMM_NULL)) {
    MPI_Comm_free(&this->communicatorCopy);
    icetDestroyContext(this->context);
  }
}

void IceTBase::updateCommunicator(MPI_Comm communicator) {
  int comparison;
  if (this->communicatorCopy != MPI_COMM_NULL) {
    MPI_Comm_compare(communicator, this->communicatorCopy, &comparison);
  } else {
    comparison = MPI_UNEQUAL;
  }

  if ((comparison != MPI_CONGRUENT) && (comparison != MPI_IDENT)) {
    // Communicator is different. Destroy old and replace.
    if (this->communicatorCopy != MPI_COMM_NULL) {
      MPI_Comm_free(&this->communicatorCopy);
      icetDestroyContext(this->context);
    }

    MPI_Comm_dup(communicator, &this->communicatorCopy);
    IceTCommunicator icetComm = icetCreateMPICommunicator(communicator);
    this->context = icetCreateContext(icetComm);
    icetDestroyMPICommunicator(icetComm);
  }
}

bool IceTBase::setOptions(const std::vector<option::Option>&,
                          MPI_Comm communicator,
                          YamlWriter&) {
  this->updateCommunicator(communicator);
  return true;
}

template <typename ImageType>
static std::unique_ptr<Image> doCompose(ImageType* localImage,
                                        YamlWriter& yaml) {
  using ImageProperties = IceTImageProperties<ImageType>;

  // Setup IceT image parameters
  icetSetColorFormat(ImageProperties::colorFormat);
  icetSetDepthFormat(ImageProperties::depthFormat);

  std::array<IceTFloat, 4> backgroundColor = {0, 0, 0, 0};

  const Viewport& validViewport = localImage->getValidViewport();
  std::array<IceTInt, 4> viewport = {validViewport.getMinX(),
                                     validViewport.getMinY(),
                                     validViewport.getWidth(),
                                     validViewport.getHeight()};

  IceTImage resultIceTImage =
      icetCompositeImage(ImageProperties::colorBuffer(*localImage),
                         ImageProperties::depthBuffer(*localImage),
                         viewport.data(),
                         nullptr,
                         nullptr,
                         backgroundColor.data());

  // Get the image data back out of IceT
  IceTInt validOffset;
  icetGetIntegerv(ICET_VALID_PIXELS_OFFSET, &validOffset);
  IceTInt numValid;
  icetGetIntegerv(ICET_VALID_PIXELS_NUM, &numValid);

  Timer copyImageTime(yaml, "icet-copy-result-seconds");
  ImageType resultImage(localImage->getWidth(),
                        localImage->getHeight(),
                        validOffset,
                        validOffset + numValid);
  using ColorType = typename ImageType::ColorType;
  const ColorType* icetColorBuffer =
      ImageProperties::colorBuffer(resultIceTImage);
  std::copy(
      icetColorBuffer + (validOffset * ImageType::ColorVecSize),
      icetColorBuffer + ((validOffset + numValid) * ImageType::ColorVecSize),
      resultImage.getColorBuffer());

  return resultImage.shallowCopy();
}

std::unique_ptr<Image> IceTBase::compose(Image* localImage,
                                         MPI_Group group,
                                         MPI_Comm communicator,
                                         YamlWriter& yaml) {
  // Make sure IceT context is up to date
  this->updateCommunicator(communicator);
  icetSetContext(this->context);

  int commSize;
  MPI_Comm_size(communicator, &commSize);

  int groupSize;
  MPI_Group_size(group, &groupSize);

  if (commSize != groupSize) {
    std::cerr << "IceT compose group must be same size as communicator."
              << std::endl;
    return localImage->copySubrange(0, 0);
  }

  if (localImage->blendIsOrderDependent()) {
    icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);
    icetEnable(ICET_ORDERED_COMPOSITE);

    MPI_Group commGroup;
    MPI_Comm_group(communicator, &commGroup);

    std::vector<int> groupOrder(groupSize);
    for (int i = 0; i < groupSize; ++i) {
      groupOrder[i] = i;
    }

    std::vector<int> compositeOrder(groupSize);
    MPI_Group_translate_ranks(
        group, groupSize, groupOrder.data(), commGroup, compositeOrder.data());

    icetCompositeOrder(compositeOrder.data());

    MPI_Group_free(&commGroup);
  } else {
    icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
    icetDisable(ICET_ORDERED_COMPOSITE);
  }

  // Make sure IceT is compositing the right image dimensions.
  icetResetTiles();
  icetAddTile(0, 0, localImage->getWidth(), localImage->getHeight(), 0);

  // Turn off IceT image collection. Let main loop collect the images to be
  // more consistent with the other miniapps.
  icetDisable(ICET_COLLECT_IMAGES);

  icetStrategy(ICET_STRATEGY_SEQUENTIAL);
  icetSingleImageStrategy(ICET_SINGLE_IMAGE_STRATEGY_AUTOMATIC);

#define TRY_IMAGE_TYPE(ImageType)                                   \
  do {                                                              \
    if (dynamic_cast<ImageType*>(localImage) != nullptr) {          \
      return doCompose(dynamic_cast<ImageType*>(localImage), yaml); \
    }                                                               \
  } while (false)
  TRY_IMAGE_TYPE(ImageRGBAFloatColorOnly);
  TRY_IMAGE_TYPE(ImageRGBAUByteColorFloatDepth);
  TRY_IMAGE_TYPE(ImageRGBAUByteColorOnly);
  TRY_IMAGE_TYPE(ImageRGBFloatColorDepth);

  std::cerr << "Image format not supported by IceT" << std::endl;
  return localImage->copySubrange(0, 0);
}

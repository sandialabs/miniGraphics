// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <miniGraphicsConfig.h>

#include <assert.h>

#include <memory>
#include <vector>

#include <Common/Color.hpp>

#include <mpi.h>

class Image {
 private:
  struct Internals {
    int width;
    int height;

    int regionBegin;
    int regionEnd;

    Internals(int _width, int _height, int _regionBegin, int _regionEnd)
        : width(_width),
          height(_height),
          regionBegin(_regionBegin),
          regionEnd(_regionEnd) {
      assert(this->width >= 0);
      assert(this->height >= 0);
      assert(this->regionBegin >= 0);
      assert(this->regionEnd <= this->width * this->height);
      assert(this->regionBegin <= this->regionEnd);
    }
  };
  Internals internals;

 public:
  Image(int _width, int _height)
      : internals(_width, _height, 0, _width * _height) {}
  Image(int _width, int _height, int _regionBegin, int _regionEnd)
      : internals(_width, _height, _regionBegin, _regionEnd) {}

  virtual ~Image();

  int getWidth() const { return this->internals.width; }
  int getHeight() const { return this->internals.height; }
  int getRegionBegin() const { return this->internals.regionBegin; }
  int getRegionEnd() const { return this->internals.regionEnd; }

  /// \brief Returns the number of (valid) pixels in the image.
  ///
  /// Note that an Image might actually only contain a subregion of pixels for
  /// the image. Thus, getNumberOfPixels might return a number smaller than
  /// width x height.
  int getNumberOfPixels() const {
    return this->getRegionEnd() - this->getRegionBegin();
  }

  /// \brief Converts x/y location in image to a pixel index.
  int pixelIndex(int x, int y) const {
    return y * this->getWidth() + x - this->getRegionBegin();
  }

  /// \brief Converts a pixel index to the x/y location.
  void xyIndices(int pixelIndex, int& x, int& y) {
    x = (pixelIndex + this->getRegionBegin()) % this->getWidth();
    y = (pixelIndex + this->getRegionBegin()) / this->getWidth();
  }

  /// \brief Clears the image to the given color and depth (if applicable).
  virtual void clear(const Color& color = Color(0, 0, 0, 0),
                     float depth = 1.0f) = 0;

  /// \brief Blend this image with another image
  ///
  /// This operation will almost certainly result in an error if the two images
  /// are not of the same type.
  ///
  /// When blending, this image is blended "on top" of the other image. Some
  /// blend operations (like z-buffer) do not depend on the blendOrder, so in
  /// those cases it will be ignored.
  virtual std::unique_ptr<Image> blend(const Image* otherImage) const = 0;

  /// \brief Returns whether blending in this buffer is order dependent.
  ///
  /// Some blending operations, such as z-buffer comparison, are order
  /// independent. That is, it does not matter in what order you do the
  /// compositing. However, other types of blending, like alpha blending,
  /// are order-dependent, and you have to be careful on the order in which
  /// you render geometry and composite images.
  ///
  virtual bool blendIsOrderDependent() const = 0;

  /// \brief Creates a new image object of the same type as this one.
  virtual std::unique_ptr<Image> createNew(int _width,
                                           int _height,
                                           int _regionBegin,
                                           int _regionEnd) const = 0;

  /// \brief Creates a new image object of the same type as this one.
  ///
  /// The new image is given the same width, height, and region as this one.
  /// The memory is allocated but no data are set.
  std::unique_ptr<Image> createNew() const {
    return this->createNew(this->getWidth(),
                           this->getHeight(),
                           this->getRegionBegin(),
                           this->getRegionEnd());
  }

  /// \brief Creates a new image containing a subrange of the given image.
  ///
  /// Allocates a new image of an appropriate size and then copies the data
  /// in the given subrange to the new image.
  ///
  /// Note that the argument \c subregionBegin and \c subregionEnd are given
  /// with respect to the region of the current image. Thus, if the current
  /// region is from pixel 100 to pixel 200 and you call \c CopySubrange from
  /// 50 to 100, you get the second 50 pixels currently held, which are in the
  /// position of pixels 150 to 200 with respect to the original image.
  virtual std::unique_ptr<Image> copySubrange(int subregionBegin,
                                              int subregionEnd) const = 0;

  std::unique_ptr<Image> deepCopy() const {
    return this->copySubrange(0, this->getNumberOfPixels());
  }

  virtual std::unique_ptr<const Image> shallowCopy() const = 0;

  std::unique_ptr<Image> shallowCopy() {
    std::unique_ptr<const Image> constCopy =
        const_cast<const Image*>(this)->shallowCopy();

    return std::unique_ptr<Image>(const_cast<Image*>(constCopy.release()));
  }

  /// \brief Sends this image to another process without blocking.
  ///
  /// The image will be sent sent to the process specified by the destination
  /// rank. This function will return right away. You should not alter the
  /// contents or delete the image until all communication finishes.
  ///
  /// The returned vector of MPI request objects can be used to wait for the
  /// send to be complete.
  virtual std::vector<MPI_Request> ISend(int destRank,
                                         MPI_Comm communicator) const = 0;

  /// \brief Sends this image to another process.
  ///
  /// The image will be sent sent to the process specified by the destination
  /// rank. This function will block until the data is sent.
  void Send(int destRank, MPI_Comm communicator) const;

  /// \brief Receives an image from another process without blocking.
  ///
  /// The image will be received from the process specified by the source rank.
  /// This function will return right away. You should not alter the contents
  /// or delete the image until all communication finishes. It is assumed that
  /// this image was constructed with a subregion at least large enough to hold
  /// all the data that is coming in.
  ///
  /// The returned vector of MPI request objects can be used to wait for the
  /// send to be complete.
  virtual std::vector<MPI_Request> IReceive(int sourceRank,
                                            MPI_Comm communicator) = 0;

  /// \brief Receives an image from another process.
  ///
  /// The image will be received from the process specified by the source rank.
  /// This function will block until the data is received. It is assumed that
  /// this image was constructed with a subregion at least large enough to hold
  /// all the data that is coming in.
  void Receive(int sourceRank, MPI_Comm communicator);

 protected:
  /// \brief Sends the metadata information for this image.
  ///
  /// This should be used internally by implementations of ISend.
  std::vector<MPI_Request> ISendMetaData(int destRank,
                                         MPI_Comm communicator) const;

  /// \brief Receives the metadata information for this image.
  ///
  /// This should be used internally by implementations of IReceive.
  std::vector<MPI_Request> IReceiveMetaData(int sourceRank,
                                            MPI_Comm communicator);
};

#endif  // IMAGE_HPP

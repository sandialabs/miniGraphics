// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "Image.hpp"

Image::~Image() {}

void Image::clear(const Color& color, float depth) {
  this->clearImpl(color, depth);
}

std::unique_ptr<Image> Image::createNew(int _width,
                                        int _height,
                                        int _regionBegin,
                                        int _regionEnd,
                                        const Viewport& _validViewport) const {
  std::unique_ptr<Image> newImage =
      this->createNewImpl(_width, _height, _regionBegin, _regionEnd);
  newImage->setValidViewport(_validViewport);
  return newImage;
}

std::unique_ptr<Image> Image::createNew(int _regionBegin,
                                        int _regionEnd) const {
  return this->createNew(this->getWidth(),
                         this->getHeight(),
                         _regionBegin,
                         _regionEnd,
                         this->getValidViewport());
}

std::unique_ptr<Image> Image::createNew() const {
  return this->createNew(this->getWidth(),
                         this->getHeight(),
                         this->getRegionBegin(),
                         this->getRegionEnd(),
                         this->getValidViewport());
}

void Image::Send(int destRank, MPI_Comm communicator) const {
  std::vector<MPI_Request> requests = this->ISend(destRank, communicator);

  MPI_Waitall(
      static_cast<int>(requests.size()), requests.data(), MPI_STATUSES_IGNORE);
}

void Image::Receive(int sourceRank, MPI_Comm communicator) {
  std::vector<MPI_Request> requests = this->IReceive(sourceRank, communicator);

  MPI_Waitall(
      static_cast<int>(requests.size()), requests.data(), MPI_STATUSES_IGNORE);
}

static const int IMAGE_INTERNALS_TAG = 59463;

std::vector<MPI_Request> Image::ISendMetaData(int destRank,
                                              MPI_Comm communicator) const {
  std::vector<MPI_Request> requests(1);

  MPI_Isend(&this->internals,
            sizeof(Image::Internals) / sizeof(int),
            MPI_INT,
            destRank,
            IMAGE_INTERNALS_TAG,
            communicator,
            &requests[0]);

  return requests;
}

std::vector<MPI_Request> Image::IReceiveMetaData(int sourceRank,
                                                 MPI_Comm communicator) {
  std::vector<MPI_Request> requests(1);

  MPI_Irecv(&this->internals,
            sizeof(Image::Internals) / sizeof(int),
            MPI_INT,
            sourceRank,
            IMAGE_INTERNALS_TAG,
            communicator,
            &requests[0]);

  return requests;
}

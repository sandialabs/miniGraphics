// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef ICETBASE_HPP
#define ICETBASE_HPP

#include <Common/Compositor.hpp>

// Prevent windows.h from defining min and max macros
#define NOMINMAX
#include <IceT.h>

class IceTBase : public Compositor {
  IceTContext context;
  MPI_Comm communicatorCopy;

  void updateCommunicator(MPI_Comm communicator);

 public:
  IceTBase();

  std::unique_ptr<Image> compose(Image *localImage,
                                 MPI_Group group,
                                 MPI_Comm communicator,
                                 YamlWriter &yaml) final;

  bool setOptions(const std::vector<option::Option> &options,
                  MPI_Comm communicator,
                  YamlWriter &yaml) final;

  virtual ~IceTBase();
};

#endif  // ICETBASE_HPP

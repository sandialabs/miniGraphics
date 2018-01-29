// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef MESHHELPER_HPP
#define MESHHELPER_HPP

// Helper functions for working with Mesh objects.

#include <Common/Mesh.hpp>

/// \brief Broadcasts the mesh from MPI rank 0 to all other meshes.
///
/// All processes of the MPI commuicator must call this method before any can
/// continue. The mesh from rank 0 is transferred to all other process, which
/// return the mesh data in their own mesh arguments.
///
/// The mesh is duplicated on all processes but translated into a grid-like
/// pattern. The overlap argument specifies how much overlap or separation is
/// given in the grid. An overlap of 0 abuts the data. An overlap of 1 writes
/// all data on top of each other. A negative overlap spaces the data out.
///
void meshBroadcast(Mesh& mesh, float overlap, MPI_Comm communicator);

/// \brief Scatters the mesh from MPI rank 0 to all other meshes.
///
/// All processes of the MPI communicator must call this method before any can
/// continue. The mesh from rank 0 is divided into even pieces, and those pieces
/// are then distributed among the processes. All processes (other than rank 0)
/// return their data through their mesh argument.
///
void meshScatter(Mesh& mesh, MPI_Comm communicator);

/// \brief Sorts a meshes triangles from front to back.
///
/// Given a mesh and a modelview matrix, returns a new Mesh object with the
/// same geometry but all the triangles reordered from front-to-back such that
/// the first triangle is the closest to the viewer and that every triangle is
/// in front of every other triangle
///
Mesh meshVisibilitySort(const Mesh& mesh, const glm::mat4& modelview);

#endif  // MESHHELPER_HPP

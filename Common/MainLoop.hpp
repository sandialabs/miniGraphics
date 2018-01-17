// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef MAINLOOP_HPP
#define MAINLOOP_HPP

#include <Common/Compositor.hpp>

#include <optionparser.h>

/// \brief Create a miniGraphics app around a compositor
///
/// Given the program arguments and a compositor to run, parse the arguments
/// and then perform a parallel rendering task (as specified by the arguments).
/// Calling this function as the implementation of your main makes a consistent
/// interface across all mini apps.
///
/// The MainLoop function can also optionally take an array or vector of
/// option parser options. These will be appended to the standard options
/// provided.
///
/// The return value is the code that should be returned from the main
/// function.
///
int MainLoop(int argc,
             char* argv[],
             Compositor* compositor,
             const option::Descriptor* compositorOptions = nullptr);
int MainLoop(int argc,
             char* argv[],
             Compositor* compositor,
             const std::vector<option::Descriptor>& compositorOptions);


// Helper functions that can be used for the CheckArg of the options parser.

// Succeeds if the option has an integer argument greater than 0.
option::ArgStatus PositiveIntArg(const option::Option& option,
                                 bool messageOnError);

// Succeeds if the option has a valid floating point argument.
option::ArgStatus FloatArg(const option::Option& option, bool messageOnError);

// Succeeds if the option has any non-empty argument.
option::ArgStatus NonemptyStringArg(const option::Option& option,
                                    bool messageOnError);

#endif  // MAINLOOP_HPP

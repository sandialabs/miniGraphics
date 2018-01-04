// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

#include "YamlWriter.hpp"

/// \brief A simple timer class that records a duration of time into a yaml
/// writer.
class Timer {
 public:
  Timer() = delete;
  Timer(const Timer&) = delete;

  /// \brief Construct a timer with the given yaml writer.
  ///
  /// Note that a reference to the YamlWriter object is recorded, so should
  /// remain alive throughout the life of this timer (and it would be best if
  /// blocks were not started or ended). For this reason, it is best to
  /// constraing the life of a timer.
  ///
  Timer(YamlWriter& _yaml);

  /// \brief Construct a timer with the given yaml writer and start timing.
  ///
  /// The given description is used as the key when recording the timing
  /// to the yaml writer.
  ///
  /// Note that a reference to the YamlWriter object is recorded, so should
  /// remain alive throughout the life of this timer (and it would be best if
  /// blocks were not started or ended). For this reason, it is best to
  /// constraing the life of a timer.
  ///
  Timer(YamlWriter& _yaml, const std::string& _description);

  /// If the Timer is running when destroyed, the timer will be stopped and
  /// the time recorded in the YamlWriter.
  ///
  ~Timer();

  bool isRunning() const;

  /// \brief Starts running the timer.
  ///
  /// The given description is used later to write the time value into the
  /// yaml when the timer is stopped.
  ///
  void start(const std::string& _description);

  /// \brief Stops running the timer and records the time value in the yaml.
  ///
  /// Will add a dictionary entry into the yaml with the description previously
  /// given when the timer was started. The time is written in seconds.
  ///
  void stop();

 private:
  YamlWriter& yaml;
  std::string description;
  std::chrono::high_resolution_clock::time_point startTime;
};

#endif  // TIMER_HPP

// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "Timer.hpp"

#include <stdexcept>

Timer::Timer(YamlWriter& _yaml) : yaml(_yaml), description("") {}

Timer::Timer(YamlWriter& _yaml, const std::string& _description)
    : yaml(_yaml), description("") {
  this->start(_description);
}

Timer::~Timer() {
  if (this->isRunning()) {
    this->stop();
  }
}

bool Timer::isRunning() const { return (this->description != ""); }

void Timer::start(const std::string& _description) {
  if (this->isRunning()) {
    throw std::runtime_error("Attempting to start an already running timer.");
  }
  this->description = _description;
  this->startTime = std::chrono::high_resolution_clock::now();
}

void Timer::stop() {
  if (!this->isRunning()) {
    throw std::runtime_error("Tried to stop a timer that is not running.");
  }
  std::chrono::high_resolution_clock::time_point endTime =
      std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> secondsElapsed = endTime - startTime;
  this->yaml.AddDictionaryEntry(this->description, secondsElapsed.count());
}

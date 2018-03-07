// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "RadixKBase.hpp"

#include <Common/MainLoop.hpp>

#include "../../DirectSend/Overlap/DirectSendOverlap.hpp"

#include <array>
#include <sstream>

constexpr int DEFAULT_TARGET_K = 8;

static std::string kToString(const std::vector<int>& kVector) {
  if (kVector.empty()) {
    return "";
  }
  std::stringstream buffer;
  for (auto&& k : kVector) {
    buffer << k << ",";
  }
  std::string s = buffer.str();
  return s.substr(0, s.length() - 1);
}

std::unique_ptr<Image> RadixKBase::compose(Image* localImage,
                                           MPI_Group group,
                                           MPI_Comm communicator,
                                           YamlWriter& yaml) {
  MPI_Group workingGroup;
  int dummy;
  MPI_Group_excl(group, 0, &dummy, &workingGroup);

  std::unique_ptr<Image> workingImage = localImage->shallowCopy();

  for (auto&& k : this->kVector) {
    int groupSize;
    MPI_Group_size(workingGroup, &groupSize);
    assert((groupSize % k) == 0);
    int myGroupRank;
    MPI_Group_rank(workingGroup, &myGroupRank);

    int mySubgroupRank = myGroupRank % k;
    int mySubgroupPartition = myGroupRank / k;

    // Grab a continuous group of k processes and do a direct send on them
    MPI_Group directSendGroup;
    std::array<int[3], 1> procRange = {
        k * mySubgroupPartition, k * (mySubgroupPartition + 1) - 1, 1};
    MPI_Group_range_incl(workingGroup, 1, procRange.data(), &directSendGroup);

    workingImage = DirectSendOverlap::compose(workingImage.get(),
                                              directSendGroup,
                                              directSendGroup,
                                              communicator,
                                              yaml);
    MPI_Group_free(&directSendGroup);

    // Collect all processes that have the same image piece into a single group
    // and decend into it
    MPI_Group subgroup;
    procRange = {mySubgroupRank, groupSize - 1, k};
    MPI_Group_range_incl(workingGroup, 1, procRange.data(), &subgroup);
    MPI_Group_free(&workingGroup);
    workingGroup = subgroup;
  }

  MPI_Group_free(&workingGroup);

  return workingImage;
}

void RadixKBase::generateK(int targetK, int numProc) {
  this->kVector.resize(0);
  int remainingProduct = numProc;
  while (remainingProduct > 1) {
    // If targetK is a factor. Just use that.
    if ((remainingProduct % targetK) == 0) {
      this->kVector.push_back(targetK);
      remainingProduct /= targetK;
      continue;
    }

    // Next, look for smaller factors.
    int k = targetK - 1;
    while ((k > 1) && ((remainingProduct % k) != 0)) {
      --k;
    }
    if (k > 1) {
      // Found a smaller factor.
      this->kVector.push_back(k);
      remainingProduct /= k;
      continue;
    }

    // There must be a larger (prime) factor. Use that.
    k = targetK + 1;
    while ((remainingProduct % k) != 0) {
      ++k;
    }
    this->kVector.push_back(k);
    remainingProduct /= k;
  }

#ifndef NDEBUG
  // Sanity check: k's should multiply to numProc
  int product = 1;
  for (auto&& k : this->kVector) {
    product *= k;
  }
  assert(product == numProc);
#endif
}

enum optionIndex { K_VALUES, TARGET_K };

std::vector<option::Descriptor> RadixKBase::getOptionVector() {
  std::vector<option::Descriptor> usage;
  // clang-format off
  usage.push_back(
    {K_VALUES, 0, "", "k", NonemptyStringArg,
     "  --k=<factors>          Set the k factors to use during compositing. The\n"
     "                         argument must be a comma or space separated list\n"
     "                         of positive integers (e.g. --k=8,8,4)."});
  usage.push_back(
    {TARGET_K, 0, "", "target-k", PositiveIntArg,
     "  --target-k=<num>       When k values are generated, it attempts to find\n"
     "                         values as near to this target k as possible. If\n"
     "                         the k values are given (with the --k option),\n"
     "                         then this argument is ignored. (Default 8).\n"});
  // clang-format on

  return usage;
}

bool RadixKBase::setOptions(const std::vector<option::Option>& options,
                            MPI_Comm communicator,
                            YamlWriter& yaml) {
  int numProc;
  MPI_Comm_size(communicator, &numProc);

  int rank;
  MPI_Comm_rank(communicator, &rank);

  if (options[K_VALUES]) {
    this->kVector.resize(0);
    std::string kArg(options[K_VALUES].arg);
    constexpr auto delimiters = " ,;:";
    std::size_t start = kArg.find_first_not_of(delimiters);
    while (start != std::string::npos) {
      std::size_t end = kArg.find_first_of(delimiters, start);
      this->kVector.push_back(atoi(kArg.substr(start, end - start).c_str()));
      start = kArg.find_first_not_of(delimiters, end);
    }

    // Check to make sure k values are valid
    int product = 1;
    for (auto&& k : this->kVector) {
      product *= k;
    }
    if (product != numProc) {
      if (rank == 0) {
        std::cerr << "Invalid k values: " << kToString(this->kVector)
                  << std::endl;
        std::cerr << "The k values must multiply to the number of processes ("
                  << numProc << ")" << std::endl;
      }
      return false;
    }
  } else {
    // Must generate our own k values
    int targetK = DEFAULT_TARGET_K;
    if (options[TARGET_K]) {
      targetK = atoi(options[TARGET_K].arg);
    }
    this->generateK(targetK, numProc);
  }
  yaml.AddDictionaryEntry("k", kToString(this->kVector));
  if (rank == 0) {
    std::cout << "k values: " << kToString(this->kVector) << std::endl;
  }

  return true;
}

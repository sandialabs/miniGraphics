#!/usr/bin/env bash

# format-source.sh: A script to run clang-format on all the source files
# in this project.

## miniGraphics is distributed under the OSI-approved BSD 3-clause License.
## See LICENSE.txt for details.
##
## Copyright (c) 2017
## National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
## the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
## certain rights in this software.

# This is a list of file patterns to match to run the formatting.
files_to_match=(
    "*.cpp"
    "*.hpp"
    "*.vertexshader"
    "*.fragmentshader"
)

# This is a list of paths to skip
paths_to_skip=(
    ThirdParty
    Utilities
)

project_dir=`dirname $0`/..

#find $project_dir -path $project_dir/ThirdParty -prune -o -name '*.cpp' -print

find_command="find $project_dir \\( -false"

for skip_pattern in "${paths_to_skip[@]}"; do
    find_command="$find_command -o -path $project_dir/$skip_pattern"
done

find_command="$find_command \\) -prune"

for match_pattern in "${files_to_match[@]}"; do
    find_command="$find_command -o -name '$match_pattern' -print"
done

#echo $find_command
eval $find_command | xargs clang-format -i

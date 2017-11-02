#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="glm"
readonly ownership="glm Upstream <no_one@nowhere.com>"
readonly subtree="ThirdParty/$name/include"
readonly repo="https://github.com/g-truc/glm.git"
readonly tag="0.9.8.5"

readonly paths="
copying.txt
readme.md

glm/*.hpp
glm/detail/*
glm/gtc/*
glm/gtx/*
glm/simd/*
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"

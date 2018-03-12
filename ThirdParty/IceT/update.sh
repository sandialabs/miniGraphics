#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="IceT"
readonly ownership="IceT Upstream <no_one@nowhere.com>"
readonly subtree="ThirdParty/$name/miniGraphics$name"
readonly repo="https://gitlab.kitware.com/icet/icet.git"
readonly tag="HEAD"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"

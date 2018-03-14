// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "YamlWriter.hpp"

#include <stdexcept>

YamlWriter::YamlWriter(std::ostream& outputStream)
    : OutputStream(outputStream), AtBlockStart(true) {
  this->BlockStack.push(Block(0));
}

YamlWriter::~YamlWriter() {
  if (this->BlockStack.size() != 1) {
    std::cerr << "YamlWriter destroyed before last block complete."
              << std::endl;
  }
}

YamlWriter::Block& YamlWriter::CurrentBlock() { return this->BlockStack.top(); }

const YamlWriter::Block& YamlWriter::CurrentBlock() const {
  return this->BlockStack.top();
}

void YamlWriter::WriteIndent() {
  int indent = this->CurrentBlock().Indent;
  bool listStart = this->CurrentBlock().AtListItemStart;

  if (listStart) {
    --indent;
  }

  for (int i = 0; i < indent; ++i) {
    this->OutputStream << "  ";
  }

  if (listStart) {
    this->OutputStream << "- ";
    this->CurrentBlock().AtListItemStart = false;
  }
}

void YamlWriter::StartBlock(const std::string& key) {
  this->WriteIndent();
  this->OutputStream << key << ":" << std::endl;

  int indent = this->CurrentBlock().Indent;
  this->BlockStack.push(Block(indent + 1));
  this->AtBlockStart = true;
}

void YamlWriter::EndBlock() {
  this->BlockStack.pop();
  this->AtBlockStart = false;

  if (this->BlockStack.empty()) {
    throw std::runtime_error("Ended a block that was never started.");
  }
}

void YamlWriter::StartListItem() {
  Block& block = this->CurrentBlock();
  if (block.IsList) {
    if (!block.AtListItemStart) {
      block.AtListItemStart = true;
    } else {
      // Ignore empty list items
    }
  } else if (this->AtBlockStart) {
    // Starting a list.
    block.IsList = true;
    block.AtListItemStart = true;
    ++block.Indent;
  } else {
    throw std::runtime_error(
        "Tried to start a list in the middle of a yaml block.");
  }
}

void YamlWriter::AddListValue(const std::string& value) {
  this->StartListItem();
  this->WriteIndent();
  this->OutputStream << value << std::endl;
  this->AtBlockStart = false;
}

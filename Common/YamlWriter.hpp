// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#ifndef _YamlWriter_h
#define _YamlWriter_h

#include <iostream>
#include <stack>
#include <string>

class YamlWriter {
  struct Block {
    int Indent;
    bool IsList;
    bool AtListItemStart;

    Block(int indent) : Indent(indent), IsList(false), AtListItemStart(false) {}
  };

  std::ostream& OutputStream;
  std::stack<Block> BlockStack;
  bool AtBlockStart;

  Block& CurrentBlock();

  const Block& CurrentBlock() const;

  void WriteIndent();

 public:
  YamlWriter(std::ostream& outputStream = std::cout);

  ~YamlWriter();

  /// Starts a block underneath a dictionary item. The key for the block is
  /// given, and the contents of the block, which can be a list or dictionary
  /// or list of dictionaries and can contain sub-blocks, is created by calling
  /// further methods of this class.
  ///
  /// A block started with \c StartBlock _must_ be ended with \c EndBlock.
  ///
  void StartBlock(const std::string& key);

  /// Finishes a block.
  ///
  void EndBlock();

  /// Start an item in a list. Can be a dictionary item.
  ///
  void StartListItem();

  /// Add a list item that is just a single value.
  ///
  void AddListValue(const std::string& value);

  /// Add a key/value pair for a dictionary entry.
  ///
  template <typename T>
  void AddDictionaryEntry(const std::string& key, const T& value) {
    this->WriteIndent();
    this->OutputStream << key << ": " << value << std::endl;
    this->AtBlockStart = false;
  }
};

#endif  //_YamlWriter_h

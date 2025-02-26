// Copyright (C) 2020-2022 Free Software Foundation, Inc.

// This file is part of GCC.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#ifndef RUST_LEX_H
#define RUST_LEX_H

#include "rust-linemap.h"
#include "rust-buffered-queue.h"
#include "rust-token.h"

namespace Rust {
// Simple wrapper for FILE* that simplifies destruction.
struct RAIIFile
{
private:
  FILE *file;
  const char *filename;

  void close ()
  {
    if (file != nullptr && file != stdin)
      fclose (file);
  }

public:
  RAIIFile (const char *filename) : filename (filename)
  {
    if (strcmp (filename, "-") == 0)
      file = stdin;
    else
      file = fopen (filename, "r");
  }

  /**
   * Create a RAIIFile from an existing instance of FILE*
   */
  RAIIFile (FILE *raw, const char *filename = nullptr)
    : file (raw), filename (filename)
  {}

  RAIIFile (const RAIIFile &other) = delete;
  RAIIFile &operator= (const RAIIFile &other) = delete;

  // have to specify setting file to nullptr, otherwise unintended fclose occurs
  RAIIFile (RAIIFile &&other) : file (other.file), filename (other.filename)
  {
    other.file = nullptr;
  }

  RAIIFile &operator= (RAIIFile &&other)
  {
    close ();
    file = other.file;
    filename = other.filename;
    other.file = nullptr;

    return *this;
  }

  ~RAIIFile () { close (); }

  FILE *get_raw () { return file; }
  const char *get_filename () { return filename; }
};

class Lexer
{
private:
  // Request new Location for current column in line_table
  Location get_current_location ();

  // Skips the current input char.
  void skip_input ();
  // Advances current input char to n + 1 chars ahead of current position.
  void skip_input (int n);

  // Returns char n chars ahead of current position.
  int peek_input ();
  // Peeks the current char.
  int peek_input (int n);

  // Classifies keyword (i.e. gets id for keyword).
  TokenId classify_keyword (const std::string &str);

  // Builds a token from the input queue.
  TokenPtr build_token ();

  std::tuple<std::string, int, bool> parse_in_decimal ();
  std::pair<std::string, int> parse_in_exponent_part ();
  std::pair<PrimitiveCoreType, int> parse_in_type_suffix ();
  std::tuple<char, int, bool> parse_escape (char opening_char);
  std::tuple<Codepoint, int, bool> parse_utf8_escape (char opening_char);
  int parse_partial_string_continue ();
  std::pair<long, int> parse_partial_hex_escape ();
  std::pair<Codepoint, int> parse_partial_unicode_escape ();

  int get_input_codepoint_length ();
  int test_get_input_codepoint_n_length (int n_start_offset);
  Codepoint peek_codepoint_input ();
  Codepoint test_peek_codepoint_input (int n);
  void skip_codepoint_input ();
  void skip_broken_string_input (int current_char);

  TokenPtr parse_byte_char (Location loc);
  TokenPtr parse_byte_string (Location loc);
  TokenPtr parse_raw_byte_string (Location loc);
  TokenPtr parse_raw_identifier (Location loc);
  TokenPtr parse_string (Location loc);
  TokenPtr maybe_parse_raw_string (Location loc);
  TokenPtr parse_raw_string (Location loc, int initial_hash_count);
  TokenPtr parse_non_decimal_int_literals (Location loc);
  TokenPtr parse_decimal_int_or_float (Location loc);
  TokenPtr parse_char_or_lifetime (Location loc);
  TokenPtr parse_identifier_or_keyword (Location loc);

  template <typename IsDigitFunc>
  TokenPtr parse_non_decimal_int_literal (Location loc,
					  IsDigitFunc is_digit_func,
					  std::string existent_str, int base);

public:
  // Construct lexer with input file and filename provided
  Lexer (const char *filename, RAIIFile input, Linemap *linemap);
  ~Lexer ();

  /**
   * Lex the contents of a string instead of a file
   */
  // FIXME: This is unsafe!
  // Since we are taking a reference to the string's internal buffer, we must
  // ensure that the lexer does not outlive the string, which might not always
  // be the case.
  //
  // We could have a fix, which would include using fmemopen() to allocate a
  // buffer and copy the string inside it.
  // ```
  // // There will be an extra nul-terminator byte written on fclose(), so
  // // account for that
  // auto string_file = fmemopen(NULL, input.length() + 1, "wr");
  // fwrite(input.c_str(), sizeof(char), input.length(), string_file);
  // auto wrapper = RAIIFile(string_file);
  // ```
  // But sadly our RAIIFile does not support moving really well... And the
  // destructor, which calls fclose(), gets called, triggering a lack of a
  // buffer to parse :)
  //
  // We need to look into fixing the RAIIFile so that it supports this
  // behaviour. I'm assuming this will be something like fixing one of the copy
  // or move constructors, but is outside of the scope of this fix. For now,
  // make sure your lexers don't live longer than the strings they're trying
  // to lex
  static Lexer lex_string (std::string &input)
  {
    // We can perform this ugly cast to a non-const char* since we're only
    // *reading* the string. This would not be valid if we were doing any
    // modification to it.
    auto string_file = fmemopen (&input[0], input.length (), "r");

    return Lexer (nullptr, RAIIFile (string_file), nullptr);
  }

  // don't allow copy semantics (for now, at least)
  Lexer (const Lexer &other) = delete;
  Lexer &operator= (const Lexer &other) = delete;

  // enable move semantics
  Lexer (Lexer &&other) = default;
  Lexer &operator= (Lexer &&other) = default;

  // Returns token n tokens ahead of current position.
  const_TokenPtr peek_token (int n) { return token_queue.peek (n); }
  // Peeks the current token.
  const_TokenPtr peek_token () { return peek_token (0); }

  // Advances current token to n + 1 tokens ahead of current position.
  void skip_token (int n) { token_queue.skip (n); }
  // Skips the current token.
  void skip_token () { skip_token (0); }

  // Replaces the current token with a specified token.
  void replace_current_token (TokenPtr replacement);
  // FIXME: don't use anymore

  /* Splits the current token into two. Intended for use with nested generics
   * closes (i.e. T<U<X>> where >> is wrongly lexed as one token). Note that
   * this will only work with "simple" tokens like punctuation. */
  void split_current_token (TokenId new_left, TokenId new_right);

  Linemap *get_line_map () { return line_map; }
  std::string get_filename () { return std::string (input.get_filename ()); }

private:
  // File for use as input.
  RAIIFile input;
  // TODO is this actually required? could just have file storage in InputSource

  // Current line number.
  int current_line;
  // Current column number.
  int current_column;
  // Current character.
  int current_char;
  // Line map.
  Linemap *line_map;

  /* Max column number that can be quickly allocated - higher may require
   * allocating new linemap */
  static const int max_column_hint = 80;

  // Input source wrapper thing.
  struct InputSource
  {
    // Input source file.
    FILE *input;

    // Create new input source from file.
    InputSource (FILE *input) : input (input) {}

    // Overload operator () to return next char from input stream.
    int operator() () { return fgetc (input); }
  };

  // The input source for the lexer.
  // InputSource input_source;
  // Input file queue.
  buffered_queue<int, InputSource> input_queue;

  // Token source wrapper thing.
  struct TokenSource
  {
    // The lexer object that will use this TokenSource.
    Lexer *lexer;

    // Create a new TokenSource with given lexer.
    TokenSource (Lexer *parLexer) : lexer (parLexer) {}

    // Overload operator () to build token in lexer.
    TokenPtr operator() () { return lexer->build_token (); }
  };

  // The token source for the lexer.
  // TokenSource token_source;
  // Token stream queue.
  buffered_queue<std::shared_ptr<Token>, TokenSource> token_queue;
};

} // namespace Rust

#endif

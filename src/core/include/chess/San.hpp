#pragma once

#include "chess/Move.hpp"
#include "chess/Position.hpp"

#include <optional>
#include <string>
#include <string_view>

// Standard Algebraic Notation: how moves are written for people, and the form
// PGN records them in.
//
// A move only has a SAN spelling relative to a position. "Nf3" names a
// different move on a different board, and whether it needs to be written
// "Nbd2" depends on what else can reach d2. Both functions therefore take the
// position the move is played from.
namespace chess::san {

// The move as it would be written: "e4", "exd5", "Nbd2", "O-O", "e8=Q+",
// "Qxf7#". The check and mate suffixes are appended by playing the move, so
// they are always right rather than guessed.
//
// The move must be legal in this position; the spelling of an illegal move is
// not meaningful.
[[nodiscard]] std::string toSan(const Position& position, const Move& move);

// Reads a SAN move, returning the matching legal move with its kind filled in.
//
// This works by spelling every legal move and comparing, so anything toSan can
// write, parse can read, and no separate grammar can drift out of step with
// it. Check and mate suffixes are optional, annotation marks such as "!?" are
// ignored, and castling may be written with zeroes.
//
// Returns nullopt when the text names no legal move.
[[nodiscard]] std::optional<Move> parse(const Position& position, std::string_view text);

} // namespace chess::san

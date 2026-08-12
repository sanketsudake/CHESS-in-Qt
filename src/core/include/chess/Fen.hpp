#pragma once

#include "chess/Position.hpp"

#include <string>
#include <string_view>
#include <variant>

// Forsyth-Edwards Notation: the standard one-line text form of a position.
// This is how tests state their starting positions, how the user interface
// offers "copy position", and how the perft suite is fed.
namespace chess::fen {

inline constexpr std::string_view kStartingPosition
    = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct ParseError {
    std::string message;
};

using ParseResult = std::variant<Position, ParseError>;

// Parses a full six-field FEN record. The halfmove clock and fullmove number
// are optional, as many published test positions omit them; they default to
// 0 and 1.
//
// Rejects, rather than silently repairing: the wrong number of ranks, a rank
// that does not describe eight squares, an unknown piece letter, a side-to-move
// field other than "w" or "b", a malformed castling or en passant field, and
// negative or non-numeric counters.
[[nodiscard]] ParseResult parse(std::string_view text);

// Serialises a position back to a six-field record. parse(serialise(p)) is p
// for every position parse accepts.
[[nodiscard]] std::string serialise(const Position& position);

} // namespace chess::fen

#pragma once

#include "chess/Game.hpp"

#include <string>

// Portable Game Notation: the text form of a whole game, which is what
// "copy game" puts on the clipboard and what other chess software reads.
//
// Export only. Reading arbitrary PGN means handling variations, comments and
// numeric annotation glyphs, and nothing in this application needs that.
namespace chess::pgn {

// The seven tags the PGN standard requires, in the order it requires them.
// The Result tag is not here because it is derived from the game rather than
// chosen.
struct Tags {
    std::string event = "Casual game";
    std::string site = "CINES";
    std::string date = "????.??.??"; // PGN's own form for an unknown date
    std::string round = "-";
    std::string white = "White";
    std::string black = "Black";
};

// The result token: "1-0", "0-1", "1/2-1/2", or "*" for a game still going.
[[nodiscard]] std::string resultToken(const Game& game);

// The whole game as PGN: the seven tag pairs, a blank line, then the movetext
// wrapped at 80 columns as the standard asks.
//
// A game that did not start from the usual position also carries the SetUp and
// FEN tags, without which the movetext cannot be replayed.
[[nodiscard]] std::string serialise(const Game& game, const Tags& tags = {});

} // namespace chess::pgn

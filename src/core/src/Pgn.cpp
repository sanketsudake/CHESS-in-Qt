#include "chess/Pgn.hpp"

#include "chess/Fen.hpp"

#include <string>

namespace chess::pgn {
namespace {

constexpr std::size_t kMaxLineLength = 80;

std::string tagPair(std::string_view name, std::string_view value)
{
    return "[" + std::string(name) + " \"" + std::string(value) + "\"]\n";
}

// Appends a token, breaking the line first when it would otherwise run past
// the standard's 80 column limit. Tokens are never split.
void appendToken(std::string& text, std::size_t& lineLength, const std::string& token)
{
    if (lineLength > 0 && lineLength + 1 + token.size() > kMaxLineLength) {
        text += '\n';
        lineLength = 0;
    } else if (lineLength > 0) {
        text += ' ';
        ++lineLength;
    }
    text += token;
    lineLength += token.size();
}

} // namespace

std::string resultToken(const Game& game)
{
    switch (game.outcome()) {
    case Outcome::WhiteWins:
        return "1-0";
    case Outcome::BlackWins:
        return "0-1";
    case Outcome::Draw:
        return "1/2-1/2";
    case Outcome::Ongoing:
        break;
    }
    return "*";
}

std::string serialise(const Game& game, const Tags& tags)
{
    const std::string result = resultToken(game);
    const Position& start = game.startPosition();
    const bool fromCustomPosition = !(start == Position::starting());

    std::string text;
    text += tagPair("Event", tags.event);
    text += tagPair("Site", tags.site);
    text += tagPair("Date", tags.date);
    text += tagPair("Round", tags.round);
    text += tagPair("White", tags.white);
    text += tagPair("Black", tags.black);
    text += tagPair("Result", result);

    // Without these two, a game that did not start from the usual position
    // cannot be replayed from its movetext.
    if (fromCustomPosition) {
        text += tagPair("SetUp", "1");
        text += tagPair("FEN", fen::serialise(start));
    }

    text += '\n';

    // Move numbers follow the starting position, so a game set up with Black
    // to move opens "12... Nf6" rather than "1. Nf6".
    int moveNumber = start.fullmoveNumber;
    bool whiteToMove = start.sideToMove == Color::White;
    std::size_t lineLength = 0;

    for (const PlayedMove& played : game.history()) {
        if (whiteToMove) {
            appendToken(text, lineLength, std::to_string(moveNumber) + ".");
        } else if (lineLength == 0) {
            // Black's move opening a line needs the number repeated, or the
            // reader cannot tell which move it is.
            appendToken(text, lineLength, std::to_string(moveNumber) + "...");
        }

        appendToken(text, lineLength, played.san);

        if (!whiteToMove) {
            ++moveNumber;
        }
        whiteToMove = !whiteToMove;
    }

    appendToken(text, lineLength, result);
    text += '\n';

    return text;
}

} // namespace chess::pgn

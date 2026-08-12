#pragma once

#include "chess/Types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace chess {

// What kind of move this is, beyond "piece goes from A to B".
//
// The generator sets this so that applying a move needs no re-derivation:
// an en passant capture removes a pawn that is not on the destination square,
// and a castle moves two pieces. Deciding that later, from the board alone,
// is exactly the sort of duplicated reasoning the 2012 code got wrong.
enum class MoveKind : std::uint8_t {
    Quiet,
    Capture,
    DoublePawnPush,
    EnPassant,
    CastleKingSide,
    CastleQueenSide,
};

struct Move {
    Square from = Square::None;
    Square to = Square::None;

    // PieceType::None unless the move promotes. Only Knight, Bishop, Rook and
    // Queen are ever stored here.
    PieceType promotion = PieceType::None;

    MoveKind kind = MoveKind::Quiet;

    [[nodiscard]] constexpr bool isValid() const { return chess::isValid(from) && chess::isValid(to); }

    [[nodiscard]] constexpr bool isPromotion() const { return promotion != PieceType::None; }

    [[nodiscard]] constexpr bool isCastle() const
    {
        return kind == MoveKind::CastleKingSide || kind == MoveKind::CastleQueenSide;
    }

    [[nodiscard]] constexpr bool isCapture() const
    {
        return kind == MoveKind::Capture || kind == MoveKind::EnPassant;
    }

    // Long algebraic notation as used by UCI: "e2e4", "e7e8q", "e1g1" for a
    // king-side castle. Promotion letters are lower case.
    [[nodiscard]] std::string toUci() const;

    // Parses the string form above. This produces from, to and promotion only
    // -- kind cannot be known without a position, so it stays Quiet. Use this
    // to look a move up in a generated move list rather than to apply it.
    [[nodiscard]] static std::optional<Move> fromUci(std::string_view text);

    // Two moves are the same move when they come from and go to the same
    // squares and promote to the same piece. Kind is derived from those in a
    // given position, so it is not part of identity.
    friend constexpr bool operator==(const Move& a, const Move& b)
    {
        return a.from == b.from && a.to == b.to && a.promotion == b.promotion;
    }
};

} // namespace chess

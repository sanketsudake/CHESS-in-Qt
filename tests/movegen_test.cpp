#include "chess/Fen.hpp"
#include "chess/MoveGenerator.hpp"
#include "chess/Rules.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace chess {
namespace {

Position mustParse(std::string_view text)
{
    fen::ParseResult result = fen::parse(text);
    if (const auto* failure = std::get_if<fen::ParseError>(&result)) {
        ADD_FAILURE() << "expected '" << text << "' to parse, got: " << failure->message;
        return Position{};
    }
    return std::get<Position>(result);
}

// Every legal move from a square, in UCI form and sorted, so a test can state
// the complete expected set rather than spot-checking one move at a time.
std::vector<std::string> legalMovesFrom(const Position& position, Square from)
{
    std::vector<std::string> found;
    for (const Move& move : generateLegalMoves(position)) {
        if (move.from == from) {
            found.push_back(move.toUci());
        }
    }
    std::sort(found.begin(), found.end());
    return found;
}

bool hasLegalMove(const Position& position, std::string_view uci)
{
    for (const Move& move : generateLegalMoves(position)) {
        if (move.toUci() == uci) {
            return true;
        }
    }
    return false;
}

const Move* findLegalMove(const Position& position, std::string_view uci)
{
    static Move storage;
    for (const Move& move : generateLegalMoves(position)) {
        if (move.toUci() == uci) {
            storage = move;
            return &storage;
        }
    }
    return nullptr;
}

TEST(MoveGenerator, StartingPositionOffersSixteenPawnAndFourKnightMoves)
{
    const MoveList moves = generateLegalMoves(Position::starting());
    EXPECT_EQ(moves.size(), 20U);
    EXPECT_TRUE(moves.contains(Square::E2, Square::E4));
    EXPECT_TRUE(moves.contains(Square::G1, Square::F3));
    EXPECT_FALSE(moves.contains(Square::E1, Square::E2));
}

TEST(MoveGenerator, PawnPushesTwoOnlyFromItsHomeRank)
{
    const Position start = Position::starting();
    EXPECT_EQ(legalMovesFrom(start, Square::E2), (std::vector<std::string>{"e2e3", "e2e4"}));

    const Position advanced = mustParse("rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    EXPECT_EQ(legalMovesFrom(advanced, Square::E3), (std::vector<std::string>{"e3e4"}));
}

TEST(MoveGenerator, PawnDoesNotJumpOverAPiece)
{
    const Position blocked = mustParse("rnbqkbnr/pppppppp/8/8/8/4n3/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_TRUE(legalMovesFrom(blocked, Square::E2).empty());
}

TEST(MoveGenerator, PawnCapturesDiagonallyAndNotForwards)
{
    const Position position = mustParse("8/8/8/8/8/3p1p2/4P3/K6k w - - 0 1");
    EXPECT_EQ(
        legalMovesFrom(position, Square::E2), (std::vector<std::string>{"e2d3", "e2e3", "e2e4", "e2f3"}));

    const Position headOn = mustParse("8/8/8/8/8/4p3/4P3/K6k w - - 0 1");
    EXPECT_TRUE(legalMovesFrom(headOn, Square::E2).empty());
}

TEST(MoveGenerator, PromotionOffersAllFourPieces)
{
    const Position position = mustParse("8/4P3/8/8/8/8/8/K6k w - - 0 1");
    EXPECT_EQ(
        legalMovesFrom(position, Square::E7), (std::vector<std::string>{"e7e8b", "e7e8n", "e7e8q", "e7e8r"}));
}

TEST(MoveGenerator, PromotionByCaptureAlsoOffersAllFourPieces)
{
    const Position position = mustParse("3r4/4P3/8/8/8/8/8/K6k w - - 0 1");
    EXPECT_EQ(legalMovesFrom(position, Square::E7),
        (std::vector<std::string>{"e7d8b", "e7d8n", "e7d8q", "e7d8r", "e7e8b", "e7e8n", "e7e8q", "e7e8r"}));
}

TEST(MoveGenerator, PromotionReplacesThePawnWithTheChosenPiece)
{
    const Position position = mustParse("8/4P3/8/8/8/8/8/K6k w - - 0 1");
    const Move* move = findLegalMove(position, "e7e8n");
    ASSERT_NE(move, nullptr);

    const Position after = applyMove(position, *move);
    EXPECT_EQ(after.board.pieceAt(Square::E8), (Piece{PieceType::Knight, Color::White}));
    EXPECT_TRUE(after.board.isEmpty(Square::E7));
}

TEST(MoveGenerator, EnPassantIsOfferedOnlyOnTheReplyToTheDoublePush)
{
    const Position afterDoublePush
        = mustParse("rnbqkbnr/pp1ppppp/8/8/2pP4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 3");
    EXPECT_TRUE(hasLegalMove(afterDoublePush, "c4d3"));

    // Same placement, but no double push just happened.
    const Position withoutTarget = mustParse("rnbqkbnr/pp1ppppp/8/8/2pP4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 3");
    EXPECT_FALSE(hasLegalMove(withoutTarget, "c4d3"));
}

TEST(MoveGenerator, EnPassantRemovesThePawnBesideTheDestination)
{
    const Position position = mustParse("rnbqkbnr/pp1ppppp/8/8/2pP4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 3");
    const Move* move = findLegalMove(position, "c4d3");
    ASSERT_NE(move, nullptr);
    EXPECT_EQ(move->kind, MoveKind::EnPassant);

    const Position after = applyMove(position, *move);
    EXPECT_EQ(after.board.pieceAt(Square::D3), (Piece{PieceType::Pawn, Color::Black}));
    EXPECT_TRUE(after.board.isEmpty(Square::D4)) << "the captured pawn stood on d4, not on d3";
    EXPECT_TRUE(after.board.isEmpty(Square::C4));
}

// Both pawns leave the fifth rank at once, so a rook that was blocked twice
// suddenly sees the king. Only a full legality check catches this; a
// pin-detection shortcut that looks at the moving pawn alone does not.
TEST(MoveGenerator, EnPassantIsIllegalWhenItDiscoversCheckAlongTheRank)
{
    const Position position = mustParse("8/8/8/K2pP2r/8/8/8/7k w - d6 0 2");
    EXPECT_FALSE(hasLegalMove(position, "e5d6"));
}

TEST(MoveGenerator, DoublePushSetsTheEnPassantTarget)
{
    const Position start = Position::starting();
    const Move* move = findLegalMove(start, "e2e4");
    ASSERT_NE(move, nullptr);
    EXPECT_EQ(move->kind, MoveKind::DoublePawnPush);
    EXPECT_EQ(applyMove(start, *move).enPassantTarget, Square::E3);

    const Move* single = findLegalMove(start, "e2e3");
    ASSERT_NE(single, nullptr);
    EXPECT_EQ(applyMove(start, *single).enPassantTarget, Square::None);
}

TEST(MoveGenerator, CastlesBothSidesWhenNothingStandsInTheWay)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    EXPECT_TRUE(hasLegalMove(position, "e1g1"));
    EXPECT_TRUE(hasLegalMove(position, "e1c1"));
}

TEST(MoveGenerator, CastlingMovesTheRookToo)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

    const Move* kingSide = findLegalMove(position, "e1g1");
    ASSERT_NE(kingSide, nullptr);
    const Position afterShort = applyMove(position, *kingSide);
    EXPECT_EQ(afterShort.board.pieceAt(Square::G1), (Piece{PieceType::King, Color::White}));
    EXPECT_EQ(afterShort.board.pieceAt(Square::F1), (Piece{PieceType::Rook, Color::White}));
    EXPECT_TRUE(afterShort.board.isEmpty(Square::H1));

    const Move* queenSide = findLegalMove(position, "e1c1");
    ASSERT_NE(queenSide, nullptr);
    const Position afterLong = applyMove(position, *queenSide);
    EXPECT_EQ(afterLong.board.pieceAt(Square::C1), (Piece{PieceType::King, Color::White}));
    EXPECT_EQ(afterLong.board.pieceAt(Square::D1), (Piece{PieceType::Rook, Color::White}));
    EXPECT_TRUE(afterLong.board.isEmpty(Square::A1));
}

TEST(MoveGenerator, CannotCastleWithoutTheRight)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1");
    EXPECT_FALSE(hasLegalMove(position, "e1g1"));
    EXPECT_FALSE(hasLegalMove(position, "e1c1"));
}

TEST(MoveGenerator, CannotCastleThroughAnOccupiedSquare)
{
    EXPECT_FALSE(hasLegalMove(mustParse("r3k2r/8/8/8/8/8/8/R3KB1R w KQkq - 0 1"), "e1g1"));
    EXPECT_FALSE(hasLegalMove(mustParse("r3k2r/8/8/8/8/8/8/RN2K2R w KQkq - 0 1"), "e1c1"));
}

TEST(MoveGenerator, CannotCastleOutOfCheck)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/4r3/R3K2R w KQ - 0 1");
    EXPECT_FALSE(hasLegalMove(position, "e1g1"));
    EXPECT_FALSE(hasLegalMove(position, "e1c1"));
}

TEST(MoveGenerator, CannotCastleThroughAnAttackedSquare)
{
    const Position kingSide = mustParse("r3k2r/8/8/8/8/8/5r2/R3K2R w KQ - 0 1");
    EXPECT_FALSE(hasLegalMove(kingSide, "e1g1"));
    EXPECT_TRUE(hasLegalMove(kingSide, "e1c1")) << "the other side is untouched";

    const Position queenSide = mustParse("r3k2r/8/8/8/8/8/3r4/R3K2R w KQ - 0 1");
    EXPECT_FALSE(hasLegalMove(queenSide, "e1c1"));
    EXPECT_TRUE(hasLegalMove(queenSide, "e1g1"));
}

TEST(MoveGenerator, CannotCastleIntoCheck)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/6r1/R3K2R w KQ - 0 1");
    EXPECT_FALSE(hasLegalMove(position, "e1g1"));
}

// Only the b-file square must be empty for the rook to pass; the king never
// stands on it, so an attack there does not prevent the castle.
TEST(MoveGenerator, QueenSideCastleIsAllowedWhenOnlyTheBFileIsAttacked)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/1r6/R3K2R w KQ - 0 1");
    EXPECT_TRUE(hasLegalMove(position, "e1c1"));
}

TEST(MoveGenerator, MovingTheKingGivesUpBothCastlingRights)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    const Move* move = findLegalMove(position, "e1e2");
    ASSERT_NE(move, nullptr);

    const Position after = applyMove(position, *move);
    EXPECT_FALSE(after.castling.has(CastlingRights::WhiteKingSide));
    EXPECT_FALSE(after.castling.has(CastlingRights::WhiteQueenSide));
    EXPECT_TRUE(after.castling.has(CastlingRights::BlackKingSide));
}

TEST(MoveGenerator, MovingARookGivesUpOnlyThatSidesRight)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    const Move* move = findLegalMove(position, "h1h2");
    ASSERT_NE(move, nullptr);

    const Position after = applyMove(position, *move);
    EXPECT_FALSE(after.castling.has(CastlingRights::WhiteKingSide));
    EXPECT_TRUE(after.castling.has(CastlingRights::WhiteQueenSide));
}

// The easy one to miss: Black never moved a piece, but its rook is gone.
TEST(MoveGenerator, CapturingARookGivesUpTheOwnersRight)
{
    const Position position = mustParse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    const Move* move = findLegalMove(position, "a1a8");
    ASSERT_NE(move, nullptr);

    const Position after = applyMove(position, *move);
    EXPECT_FALSE(after.castling.has(CastlingRights::BlackQueenSide));
    EXPECT_TRUE(after.castling.has(CastlingRights::BlackKingSide));
}

TEST(MoveGenerator, APinnedPieceMayNotAbandonTheKing)
{
    // The white knight on e2 shields the king from the rook on e8.
    const Position position = mustParse("4r3/8/8/8/8/8/4N3/4K3 w - - 0 1");
    EXPECT_TRUE(legalMovesFrom(position, Square::E2).empty());
}

TEST(MoveGenerator, APinnedPieceMayStillMoveAlongThePin)
{
    const Position position = mustParse("4r3/8/8/8/8/8/4R3/4K3 w - - 0 1");
    const std::vector<std::string> moves = legalMovesFrom(position, Square::E2);
    EXPECT_EQ(moves, (std::vector<std::string>{"e2e3", "e2e4", "e2e5", "e2e6", "e2e7", "e2e8"}));
}

TEST(MoveGenerator, WhenInCheckOnlyMovesThatAnswerItAreLegal)
{
    // Black's rook on e8 checks the white king; White may block on e4, capture
    // nothing, or step aside.
    const Position position = mustParse("4r3/8/8/8/8/8/8/4K3 w - - 0 1");
    const MoveList moves = generateLegalMoves(position);
    for (const Move& move : moves) {
        EXPECT_NE(fileOf(move.to), File::E) << "king stayed on the checked file: " << move.toUci();
    }
    EXPECT_FALSE(moves.empty());
}

TEST(MoveGenerator, KingsMayNotStandBesideEachOther)
{
    // Kings two ranks apart: d2 would put them side by side, so it is out.
    const Position position = mustParse("8/8/8/8/8/3k4/8/3K4 w - - 0 1");
    EXPECT_FALSE(hasLegalMove(position, "d1d2"));
    EXPECT_FALSE(hasLegalMove(position, "d1c2"));
    EXPECT_FALSE(hasLegalMove(position, "d1e2"));
    EXPECT_TRUE(hasLegalMove(position, "d1c1"));
    EXPECT_TRUE(hasLegalMove(position, "d1e1"));
}

TEST(MoveGenerator, PseudoLegalIncludesMovesThatLeaveTheKingInCheck)
{
    const Position position = mustParse("4r3/8/8/8/8/8/4N3/4K3 w - - 0 1");

    bool pseudoLegalHasKnightMove = false;
    for (const Move& move : generatePseudoLegalMoves(position)) {
        if (move.from == Square::E2) {
            pseudoLegalHasKnightMove = true;
        }
    }
    EXPECT_TRUE(pseudoLegalHasKnightMove);
    EXPECT_TRUE(legalMovesFrom(position, Square::E2).empty());
}

TEST(MoveList, FindMatchesOnFromToAndPromotion)
{
    const Position position = mustParse("8/4P3/8/8/8/8/8/K6k w - - 0 1");
    const MoveList moves = generateLegalMoves(position);

    ASSERT_TRUE(moves.find(Square::E7, Square::E8, PieceType::Queen).has_value());
    ASSERT_TRUE(moves.find(Square::E7, Square::E8, PieceType::Knight).has_value());
    EXPECT_EQ(moves.find(Square::E7, Square::E8, PieceType::Queen)->promotion, PieceType::Queen);
    EXPECT_FALSE(moves.find(Square::E7, Square::E8).has_value()) << "a promotion needs its piece named";
    EXPECT_FALSE(moves.find(Square::A1, Square::A2, PieceType::Queen).has_value());
}

// find returns by value on purpose. Writing the call against a temporary list
// is the natural thing to do, and a returned pointer would dangle at the
// semicolon. Address sanitizer caught exactly this in Game::play.
TEST(MoveList, FindSurvivesBeingCalledOnATemporaryList)
{
    const Position position = mustParse("8/4P3/8/8/8/8/8/K6k w - - 0 1");
    const auto found = generateLegalMoves(position).find(Square::E7, Square::E8, PieceType::Queen);

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->from, Square::E7);
    EXPECT_EQ(found->promotion, PieceType::Queen);
}

TEST(ApplyMove, FiftyMoveCounterResetsOnPawnMovesAndCaptures)
{
    const Position quiet = mustParse("8/8/8/8/8/5n2/8/K6k b - - 10 20");
    const Move* knightMove = findLegalMove(quiet, "f3g1");
    ASSERT_NE(knightMove, nullptr);
    EXPECT_EQ(applyMove(quiet, *knightMove).halfmoveClock, 11);

    const Position pawn = mustParse("8/8/8/8/8/8/4P3/K6k w - - 10 20");
    const Move* pawnMove = findLegalMove(pawn, "e2e3");
    ASSERT_NE(pawnMove, nullptr);
    EXPECT_EQ(applyMove(pawn, *pawnMove).halfmoveClock, 0);

    const Position capture = mustParse("7k/8/8/8/8/6n1/8/K5R1 w - - 10 20");
    const Move* captureMove = findLegalMove(capture, "g1g3");
    ASSERT_NE(captureMove, nullptr);
    EXPECT_EQ(applyMove(capture, *captureMove).halfmoveClock, 0);
}

TEST(ApplyMove, FullmoveNumberAdvancesAfterBlackMoves)
{
    const Position white = Position::starting();
    const Move* whiteMove = findLegalMove(white, "e2e4");
    ASSERT_NE(whiteMove, nullptr);
    const Position afterWhite = applyMove(white, *whiteMove);
    EXPECT_EQ(afterWhite.fullmoveNumber, 1);
    EXPECT_EQ(afterWhite.sideToMove, Color::Black);

    const Move* blackMove = findLegalMove(afterWhite, "e7e5");
    ASSERT_NE(blackMove, nullptr);
    const Position afterBlack = applyMove(afterWhite, *blackMove);
    EXPECT_EQ(afterBlack.fullmoveNumber, 2);
    EXPECT_EQ(afterBlack.sideToMove, Color::White);
}

} // namespace
} // namespace chess

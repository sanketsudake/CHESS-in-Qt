#include "chess/Fen.hpp"
#include "chess/MoveGenerator.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>

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

// The six positions the Chess Programming Wiki publishes perft node counts for.
// They are chosen between them to exercise every rule that is easy to get
// wrong: Kiwipete is dense with castling and pinned pieces, position 3 is an
// en-passant and promotion race, position 4 has promotions with check, and
// positions 5 and 6 are known to catch castling-rights bugs.
//
// Because the expected counts are exact, a single mishandled case shows up as
// a wrong number here rather than as a subtle misbehaviour during a game.
struct PerftCase {
    std::string_view name;
    std::string_view fenText;
    int depth;
    std::uint64_t expected;
};

constexpr std::string_view kStart = fen::kStartingPosition;
constexpr std::string_view kKiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
constexpr std::string_view kPosition3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
constexpr std::string_view kPosition4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
constexpr std::string_view kPosition5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
constexpr std::string_view kPosition6
    = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

class Perft : public testing::TestWithParam<PerftCase> { };

// Named rather than written inline at each instantiation. INSTANTIATE_TEST_SUITE_P
// expands its name-generator argument inside a function that already has a
// parameter called "info", so an inline lambda taking "info" shadows it --
// which gcc rejects under -Wshadow even though clang and MSVC accept it.
std::string perftCaseName(const testing::TestParamInfo<PerftCase>& parameter)
{
    return std::string(parameter.param.name) + "_depth" + std::to_string(parameter.param.depth);
}

TEST_P(Perft, MatchesThePublishedNodeCount)
{
    const PerftCase& testCase = GetParam();
    EXPECT_EQ(perft(mustParse(testCase.fenText), testCase.depth), testCase.expected)
        << testCase.name << " at depth " << testCase.depth;
}

// Every position is taken to depth 4, and the start position and position 3 to
// depth 5. That is deep enough for castling, en passant and promotion to
// interact, which is where generators usually break. It costs about a second
// in an optimised build and about fifteen in a debug one.
INSTANTIATE_TEST_SUITE_P(Standard, Perft,
    testing::Values(PerftCase{"start", kStart, 1, 20}, PerftCase{"start", kStart, 2, 400},
        PerftCase{"start", kStart, 3, 8902}, PerftCase{"start", kStart, 4, 197281},
        PerftCase{"start", kStart, 5, 4865609}, PerftCase{"kiwipete", kKiwipete, 1, 48},
        PerftCase{"kiwipete", kKiwipete, 2, 2039}, PerftCase{"kiwipete", kKiwipete, 3, 97862},
        PerftCase{"kiwipete", kKiwipete, 4, 4085603}, PerftCase{"position3", kPosition3, 1, 14},
        PerftCase{"position3", kPosition3, 2, 191}, PerftCase{"position3", kPosition3, 3, 2812},
        PerftCase{"position3", kPosition3, 4, 43238}, PerftCase{"position3", kPosition3, 5, 674624},
        PerftCase{"position4", kPosition4, 1, 6}, PerftCase{"position4", kPosition4, 2, 264},
        PerftCase{"position4", kPosition4, 3, 9467}, PerftCase{"position4", kPosition4, 4, 422333},
        PerftCase{"position5", kPosition5, 1, 44}, PerftCase{"position5", kPosition5, 2, 1486},
        PerftCase{"position5", kPosition5, 3, 62379}, PerftCase{"position5", kPosition5, 4, 2103487},
        PerftCase{"position6", kPosition6, 1, 46}, PerftCase{"position6", kPosition6, 2, 2079},
        PerftCase{"position6", kPosition6, 3, 89890}, PerftCase{"position6", kPosition6, 4, 3894594}),
    perftCaseName);

#ifdef CINES_SLOW_TESTS
// Hundreds of millions of nodes. Configure with -DCINES_SLOW_TESTS=ON to
// include them; CI does not. They have been run and pass.
INSTANTIATE_TEST_SUITE_P(Deep, Perft,
    testing::Values(PerftCase{"start", kStart, 6, 119060324}, PerftCase{"kiwipete", kKiwipete, 5, 193690690},
        PerftCase{"position3", kPosition3, 6, 11030083}, PerftCase{"position4", kPosition4, 5, 15833292}),
    perftCaseName);
#endif

TEST(Perft, DepthZeroCountsThePositionItself)
{
    EXPECT_EQ(perft(Position::starting(), 0), 1U);
}

} // namespace
} // namespace chess

add_library(chesscore STATIC
  src/Types.cpp
  src/Board.cpp
  src/Position.cpp
  src/Move.cpp
  src/Fen.cpp
  src/Rules.cpp
  src/MoveGenerator.cpp
  src/San.cpp
  src/Game.cpp
  src/Pgn.cpp
)

add_library(cines::core ALIAS chesscore)

target_include_directories(chesscore PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(chesscore PRIVATE cines_warnings)

# The whole point of this library is that the rules are testable without a GUI.
# Nothing here may include a Qt header.
set_target_properties(chesscore PROPERTIES
  POSITION_INDEPENDENT_CODE ON)

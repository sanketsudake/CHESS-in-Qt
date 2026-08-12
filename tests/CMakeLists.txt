include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE
  SYSTEM
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# One test binary per area of the core. Keeping them separate means a failure
# names the area, and ctest can run them in parallel.
function(cines_add_core_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE cines::core cines_warnings GTest::gtest_main)
  gtest_discover_tests(${name}
    PROPERTIES LABELS core
    DISCOVERY_TIMEOUT 30)
endfunction()

cines_add_core_test(core_types_test types_test.cpp)
cines_add_core_test(core_board_test board_test.cpp)
cines_add_core_test(core_move_test move_test.cpp)
cines_add_core_test(core_fen_test fen_test.cpp)

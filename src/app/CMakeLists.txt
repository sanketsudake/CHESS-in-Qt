find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Svg SvgWidgets)

qt_standard_project_setup()

set(cines_piece_svgs)
foreach(piece pawn knight bishop rook queen king)
  foreach(colour white black)
    list(APPEND cines_piece_svgs "${CMAKE_SOURCE_DIR}/assets/pieces/${piece}_${colour}.svg")
  endforeach()
endforeach()

# Everything except main.cpp lives in a library so the UI tests can drive the
# same widgets the application does. A test target cannot link an executable.
#
# SvgWidgets is here because QGraphicsSvgItem lives in that module: the board
# renders from SVG at whatever scale the window happens to be, rather than from
# pixmaps that would blur when resized.
qt_add_library(cines_app STATIC
  MainWindow.cpp
  MainWindow.hpp
  BoardScene.cpp
  BoardScene.hpp
  BoardView.cpp
  BoardView.hpp
  BoardGeometry.hpp
  GameController.cpp
  GameController.hpp
  PieceRenderer.cpp
  PieceRenderer.hpp
  PromotionDialog.cpp
  PromotionDialog.hpp
  Theme.cpp
  Theme.hpp
)

qt_add_resources(cines_app "pieces"
  PREFIX "/pieces"
  BASE "${CMAKE_SOURCE_DIR}/assets/pieces"
  FILES ${cines_piece_svgs}
)

target_include_directories(cines_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(cines_app PUBLIC
  cines::core
  Qt6::Widgets
  Qt6::Svg
  Qt6::SvgWidgets
)

target_link_libraries(cines_app PRIVATE cines_warnings)

target_compile_definitions(cines_app PUBLIC
  CINES_VERSION_STRING="${PROJECT_VERSION}"
  # Catches string literals implicitly converted to QString, which is an
  # allocation per call that the compiler can point out for free.
  QT_NO_CAST_FROM_ASCII
)

qt_add_executable(cines main.cpp)
target_link_libraries(cines PRIVATE cines_app cines_warnings)

# Renders a position to a PNG with no window, which is how the README
# screenshot is produced and how a board change can be looked at on a machine
# with no display.
qt_add_executable(cines_board_snapshot "${CMAKE_SOURCE_DIR}/tools/board_snapshot.cpp")
target_link_libraries(cines_board_snapshot PRIVATE cines_app cines_warnings)
set_target_properties(cines_board_snapshot PROPERTIES MACOSX_BUNDLE FALSE)

# --- Packaging metadata -----------------------------------------------------
#
# Set here rather than in the release workflow so the bundle is correct from
# the start: macdeployqt needs a real .app to work on, and linuxdeploy cannot
# build an AppDir without a desktop entry and an icon.

if(APPLE)
  set_target_properties(cines PROPERTIES
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME "CINES"
    MACOSX_BUNDLE_BUNDLE_NAME "CINES"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.sanketsudake.cines"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "cines.icns"
    MACOSX_BUNDLE_COPYRIGHT "GPL-3.0-or-later"
  )

  set(cines_icns "${CMAKE_SOURCE_DIR}/packaging/cines.icns")
  if(EXISTS "${cines_icns}")
    target_sources(cines PRIVATE "${cines_icns}")
    set_source_files_properties("${cines_icns}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  endif()

elseif(WIN32)
  set_target_properties(cines PROPERTIES WIN32_EXECUTABLE TRUE)
  set(cines_rc "${CMAKE_SOURCE_DIR}/packaging/cines.rc")
  if(EXISTS "${cines_rc}")
    target_sources(cines PRIVATE "${cines_rc}")
  endif()

else()
  include(GNUInstallDirs)
  install(TARGETS cines RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES "${CMAKE_SOURCE_DIR}/packaging/cines.desktop"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
  install(FILES "${CMAKE_SOURCE_DIR}/packaging/cines.png"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps)
endif()

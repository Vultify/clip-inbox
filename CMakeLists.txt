cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)
option(ENABLE_TESTS "Build the Clip Inbox tests" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.cpp
    src/clip.hpp
    src/clip-events.cpp
    src/clip-events.hpp
    src/clip-naming.cpp
    src/clip-naming.hpp
    src/clip-row.cpp
    src/clip-row.hpp
    src/clip-settings.cpp
    src/clip-settings.hpp
    src/clip-store.cpp
    src/clip-store.hpp
    src/com-scope.hpp
    src/file-actions.cpp
    src/file-actions.hpp
    src/file-info.cpp
    src/file-info.hpp
    src/file-ready.cpp
    src/file-ready.hpp
    src/flow-layout.cpp
    src/flow-layout.hpp
    src/inbox-dock.cpp
    src/inbox-dock.hpp
    src/settings-dialog.cpp
    src/settings-dialog.hpp
)

# The Windows shell APIs behind thumbnails, clip lengths, and the Recycle Bin.
if(WIN32)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE shell32 ole32)
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

add_executable(
  clip-inbox-tests
  main.cpp
  test-support.hpp
  clip-naming-tests.cpp
  clip-store-tests.cpp
  file-actions-tests.cpp
  file-info-tests.cpp
  file-ready-tests.cpp
  ../src/clip.hpp
  ../src/clip-naming.cpp
  ../src/clip-naming.hpp
  ../src/clip-store.cpp
  ../src/clip-store.hpp
  ../src/com-scope.hpp
  ../src/file-actions.cpp
  ../src/file-actions.hpp
  ../src/file-info.cpp
  ../src/file-info.hpp
  ../src/file-ready.cpp
  ../src/file-ready.hpp
)
target_include_directories(clip-inbox-tests PRIVATE ../src)
target_link_libraries(clip-inbox-tests PRIVATE Qt6::Core Qt6::Gui)
if(WIN32)
  target_link_libraries(clip-inbox-tests PRIVATE shell32 ole32)
endif()
set_target_properties(clip-inbox-tests PROPERTIES AUTOMOC ON)

add_test(NAME clip-inbox-tests COMMAND clip-inbox-tests)

# The tests run against the Qt that ships in .deps, so put its DLLs on the path.
set_tests_properties(
  clip-inbox-tests
  PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
)

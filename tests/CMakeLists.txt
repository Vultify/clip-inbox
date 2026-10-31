add_executable(clip-inbox-tests file-ready-tests.cpp ../src/file-ready.cpp ../src/file-ready.hpp ../src/clip.hpp)
target_include_directories(clip-inbox-tests PRIVATE ../src)
target_link_libraries(clip-inbox-tests PRIVATE Qt6::Core)
set_target_properties(clip-inbox-tests PROPERTIES AUTOMOC ON)

add_test(NAME clip-inbox-tests COMMAND clip-inbox-tests)

# The tests run against the Qt that ships in .deps, so put its DLLs on the path.
set_tests_properties(
  clip-inbox-tests
  PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
)

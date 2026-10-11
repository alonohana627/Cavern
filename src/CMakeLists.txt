add_subdirectory(game)
add_subdirectory(app)

# The executable is only main.cpp; everything else lives in the libraries.
add_executable(cavern main.cpp)
target_link_libraries(cavern PRIVATE cavern_app)
gamelib_target_warnings(cavern)
gamelib_copy_runtime(cavern)

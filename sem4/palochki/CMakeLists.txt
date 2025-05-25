add_executable(main_server main_server.cpp shmem.h semafor.h Process_compiler.h)
add_executable(client client.cpp)
add_executable(game game_subserver.cpp)
add_executable(compile compile_subserver.cpp)
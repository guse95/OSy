
set(SOURCES
        Logger.h
        Generator_Analyzer.cpp
)


add_compile_options(-Wall -Werror -Wextra -pedantic -fsanitize=address -fsanitize=leak)
add_link_options(-fsanitize=address -fsanitize=leak)
add_executable(myGenerator ${SOURCES})
add_executable(myLogger Logger.cpp)

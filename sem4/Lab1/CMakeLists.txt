
add_compile_options(-Wall -Werror -Wextra -pedantic -fsanitize=address -fsanitize=leak)
add_link_options(-fsanitize=address -fsanitize=leak)
add_executable(lr1_1 lr1_1.c)
add_executable(lr1_2 lr1_2.c)
add_executable(lr1_3 lr1_3.c)
add_executable(lr1_5 lr1_5.c)
add_executable(lr1_7 lr1_7.c)
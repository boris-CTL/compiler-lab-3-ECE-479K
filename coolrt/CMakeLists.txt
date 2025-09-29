add_library(coolrt coolrt.cpp coolrt.h)
target_compile_options(coolrt PRIVATE -O3 -ggdb3 -fsanitize=address,undefined -fsanitize-address-use-after-scope)
add_library(lwrb src/lwrb.cpp)
target_include_directories(lwrb PUBLIC include)
target_compile_options(lwrb PRIVATE -Wno-deprecated-declarations)

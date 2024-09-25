add_library(lwrb src/lwrb.cpp)
# target_compile_definitions(lwrb PUBLIC LWRB_DISABLE_ATOMIC)
target_include_directories(lwrb PUBLIC include)

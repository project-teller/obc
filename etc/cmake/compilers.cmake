include(CheckCCompilerFlag)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE)
if(FORCE_COLORED_OUTPUT)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        add_compile_options(-fcolor-diagnostics)
    endif()
endif()

# Create custom properties called CXX_EXCEPTIONS, CXX_RTTI and so on
# These get placed at global, directory and target scopes
foreach(scope GLOBAL DIRECTORY TARGET)
    define_property(
        ${scope} PROPERTY "CXX_EXCEPTIONS" INHERITED
        BRIEF_DOCS "Enable C++ exceptions, defaults to TRUE at global scope"
        FULL_DOCS "Enable C++ exceptions, defaults to TRUE at global scope"
    )
    define_property(
        ${scope} PROPERTY "CXX_RTTI" INHERITED
        BRIEF_DOCS "Enable C++ runtime type information, defaults to TRUE at global scope"
        FULL_DOCS "Enable C++ runtime type information, defaults to TRUE at global scope"
    )
    define_property(
        ${scope} PROPERTY "CXX_WARNINGS" INHERITED
        BRIEF_DOCS "Controls the warning level of compilers, defaults to TRUE at global scope"
        FULL_DOCS "Controls the warning level of compilers, defaults to TRUE at global scope"
    )
    define_property(
        ${scope} PROPERTY "CXX_WARNINGS_AS_ERRORS" INHERITED
        BRIEF_DOCS "Treat warnings as errors and abort compilation on a warning, defaults to FALSE at global scope"
        FULL_DOCS "Treat warnings as errors and abort compilation on a warning, defaults to FALSE at global scope"
    )
endforeach()

# Set the default for these properties at global scope. If they are not set per target or
# whatever, the next highest scope will be looked up
option(CMAKE_CXX_EXCEPTIONS "Enable C++ exceptions, defaults to TRUE at global scope" TRUE)
option(CMAKE_CXX_RTTI "Enable C++ runtime type information, defaults to TRUE at global scope" TRUE)
option(CMAKE_CXX_WARNINGS "Controls the warning level of compilers, defaults to TRUE at global scope" TRUE)
option(CMAKE_CXX_WARNINGS_AS_ERRORS "Treat warnings as errors and abort compilation on a warning, defaults to FALSE at global scope" FALSE)

set_property(GLOBAL PROPERTY CXX_EXCEPTIONS ${CMAKE_CXX_EXCEPTIONS})
set_property(GLOBAL PROPERTY CXX_RTTI ${CMAKE_CXX_RTTI})
set_property(GLOBAL PROPERTY CXX_WARNINGS ${CMAKE_CXX_WARNINGS})
set_property(GLOBAL PROPERTY CXX_WARNINGS_AS_ERRORS ${CMAKE_CXX_WARNINGS_AS_ERRORS})

add_compile_options(
    $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_EXCEPTIONS>>,FALSE>:-fno-exceptions>
    $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_RTTI>>,FALSE>:-fno-rtti>
    $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,TRUE>:-Wall>
    $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,FALSE>:-w>
    $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,ALL>:-Wall>
    $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,ALL>:-pedantic>
    $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS_AS_ERRORS>>,TRUE>:-Werror>
)
if(CMAKE_${COMPILER}_COMPILER_ID MATCHES "Clang" OR CMAKE_${COMPILER}_COMPILER_ID MATCHES "AppleClang")
    add_compile_options(
        $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,ALL>:-Weverything>
        $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,ALL>:-Wno-macro-redefined>
        $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,ALL>:-Wall>
        $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,ALL>:-Wno-c++98-compat>
        $<$<STREQUAL:$<UPPER_CASE:$<TARGET_PROPERTY:CXX_WARNINGS>>,ALL>:-Wno-c++98-compat-pedantic>
    )
endif()

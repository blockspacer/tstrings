cmake_minimum_required(VERSION 2.8.12)

# Make the scripts available in the 'cmake' directory available for the
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)

set (tgt tstrings_cpp)

project (${tgt} C CXX)

# cmake options
option ("${tgt}_WITH_TESTS" "Enable unit tests" OFF)

# version
set (${tgt}_VERSION_MAJOR 0)
set (${tgt}_VERSION_MINOR 1)

set (SRC
    # header-only
)

# create the library
add_library (${tgt} STATIC ${SRC})
set_target_properties(${tgt} PROPERTIES LINKER_LANGUAGE CXX)

# Turn on all warnings when using gcc.
if (CMAKE_COMPILER_IS_GNUCXX)
   add_definitions ("--std=c++11 -W -Wall")
endif ()

# project includes
target_include_directories (${tgt}
    # public includes
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    # private includes
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if ("${tgt}_WITH_TESTS")
    add_subdirectory (test)
endif ()
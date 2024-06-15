find_path(GUROBI_INCLUDE_PATH
        NAMES gurobi_c.h
        HINTS ${GUROBI_PATH} $ENV{GUROBI_HOME}
        PATH_SUFFIXES include)

find_library(GUROBI_LIBRARY
        NAMES gurobi gurobi100
        HINTS ${GUROBI_PATH} $ENV{GUROBI_HOME}
        PATH_SUFFIXES lib)

find_library(GUROBI_CXX_LIBRARY
        NAMES gurobi_c++
        HINTS ${GUROBI_PATH} $ENV{GUROBI_HOME}
        PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
include_directories(${GUROBI_INCLUDE_PATH})
find_package_handle_standard_args(GUROBI DEFAULT_MSG GUROBI_LIBRARY)
find_package_handle_standard_args(GUROBI DEFAULT_MSG GUROBI_CXX_LIBRARY)
set(AppleClang_MinVersion 11.0.0)
set(Clang_MinVersion 11.0.0)
set(GNU_MinVersion 10.0.0)
set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fcoroutines-ts)
    message(NOTICE "mpi-application: using clang, enable corresponding coroutine support")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-fcoroutines)
    message(NOTICE "mpi-application: using gcc, enable corresponding coroutine support")
else ()
    message(SEND_ERROR "mpi-application: unsupported c++ compiler, requires either gcc or clang with c++20 coroutines support")
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(GUROBI_PATH /opt/gurobi/linux64)

find_package(GUROBI REQUIRED)

if (GUROBI_FOUND)
    message(NOTICE "mpi-application: gurobi found")
    message(NOTICE "mpi-application: if encounter undefined reference errors while linking gurobi, try to recompile gurobi static library to resolve API incompatibility issues")
else ()
    message(SEND_ERROR "mpi-application: gurobi not found")
endif ()

add_library(simulator-routing
        SHARED
        routing/ipv4-routing-utility.cpp
        routing/congestion-monitor-application.cpp
        topology/dragonfly-topology.cpp
        topology/flexfly-topology.cpp
)

add_library(simulator-mip
        SHARED
        mip-model/network.cpp
)

add_executable(
        simulator
        dragonfly-test.cpp
)


target_compile_options(
        simulator
        PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
)

target_include_directories(simulator PRIVATE ${Python_INCLUDE_DIRS})

target_link_options(
        simulator
        PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
)

target_link_libraries(
        simulator
        simulator-mip
        simulator-routing
        ${GUROBI_LIBRARY}
        ${GUROBI_CXX_LIBRARY}
        ${libapplications}
        ${libcore}
        ${libnetwork}
        ${libinternet}
        ${libpoint-to-point}
        ${libmpi-application}
        ${libflow-monitor}
        ${libopenflow}
        ${libcsma}
)
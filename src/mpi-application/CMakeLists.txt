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


build_lib(
        LIBNAME mpi-application
        IGNORE_PCH
        SOURCE_FILES
            model/mpi-application.cpp
            model/mpi-communicator.cpp
            model/mpi-functions.cpp
            model/mpi-socket.cpp
            model/mpi-util.cpp
        HEADER_FILES
            model/mpi-application.h
            model/mpi-communicator.h
            model/mpi-datatype.h
            model/mpi-exception.h
            model/mpi-functions.h
            model/mpi-operation.h
            model/mpi-operation-awaitable.h
            model/mpi-operation-trait.h
            model/mpi-operation-type.h
            model/mpi-protocol.h
            model/mpi-protocol-trait.h
            model/mpi-socket.h
            model/mpi-util.h
        LIBRARIES_TO_LINK
            ${libcore}
            ${libnetwork}
            ${libinternet}
)

add_executable(
        mpi-application-test test/test.cpp
)

target_link_libraries(
        mpi-application-test
        ${libmpi-application}
        ${libcore}
        ${libnetwork}
        ${libinternet}
)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(rvision CXX)

enable_testing()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)

find_package(SQLite3 REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Poco REQUIRED Foundation Util Net)
find_package(Threads REQUIRED)
find_package(gRPC CONFIG QUIET)
find_package(GTest REQUIRED)
                                                                                                                   
if(gRPC_FOUND)
    message(STATUS "Found gRPC++: ${gRPC_VERSION}")
    #set(GRPCPP_INCLUDE_DIRS $<TARGET_PROPERTY:gRPC::grpc++,INTERFACE_INCLUDE_DIRECTORIES>)
    set(GRPCPP_LIBRARIES gRPC::grpc++)
    set(GRPCPP_PLUGIN_EXEC $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GRPCPP REQUIRED QUIET grpc++_unsecure)
    set(GRPCPP_PLUGIN_EXEC ${GRPCPP_PREFIX}/bin/grpc_cpp_plugin)
    message(STATUS "Found gRPC++: ${GRPCPP_LINK_LIBRARIES} (found version \"${GRPCPP_VERSION}\")")
endif()

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(RPC_DIR ${CMAKE_SOURCE_DIR}/src/rpc)
set(CONTRIB_DIR ${CMAKE_SOURCE_DIR}/contrib)

get_filename_component(PROTO_FILE "${RPC_DIR}/messages/rvision.proto" ABSOLUTE)
get_filename_component(PROTO_PATH "${PROTO_FILE}" PATH)
get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)

#generated sources
set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
set(GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
set(GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")

add_custom_command(
    OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${PROTO_PATH}"
        --plugin=protoc-gen-grpc="${GRPCPP_PLUGIN_EXEC}"
        "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}")

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE "$<BUILD_INTERFACE:${CONTRIB_DIR}/spdlog/include>")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${Boost_INCLUDE_DRS})

link_directories(${Boost_LIBRARY_DIRS})

set(RVISON_SRCS
				${SRC_DIR}/core/error.cpp
				${SRC_DIR}/app/app.cpp
				${SRC_DIR}/http/server/server.cpp
				${SRC_DIR}/http/server/handler.cpp
				${SRC_DIR}/http/server/dispatcher.cpp
				${SRC_DIR}/http/server/factory.cpp
				${SRC_DIR}/http/utils/params.cpp
				${SRC_DIR}/http/utils/uri.cpp
				${SRC_DIR}/rpc/server/server.cpp
				${SRC_DIR}/rpc/client/client.cpp
				${SRC_DIR}/lines/db/lines_db.cpp
				${SRC_DIR}/lines/poller/lines_poller.cpp
				${SRC_DIR}/lines/provider/lines_provider.cpp)
				
set(RVISON_HDRS
				${SRC_DIR}/core/headers.hpp
				${SRC_DIR}/core/error.hpp
				${SRC_DIR}/core/logger.hpp
				${SRC_DIR}/app/app.hpp
				${SRC_DIR}/http/server/server.hpp
				${SRC_DIR}/http/server/handler.hpp
				${SRC_DIR}/http/server/dispatcher.hpp
				${SRC_DIR}/http/server/factory.hpp
				${SRC_DIR}/http/utils/params.hpp
				${SRC_DIR}/http/utils/uri.hpp
				${SRC_DIR}/http/utils/executor.hpp
				${SRC_DIR}/rpc/server/server.hpp
				${SRC_DIR}/rpc/client/client.hpp
				${SRC_DIR}/lines/db/lines_db.hpp
				${SRC_DIR}/lines/poller/lines_poller.hpp
				${SRC_DIR}/lines/provider/lines_provider.hpp
				${SRC_DIR}/lines/utils/line_info.hpp )
				
add_executable(rvision
				${SRC_DIR}/main/main.cpp
				${RVISON_HDRS}
				${RVISON_SRCS}
				${PROTO_SRCS}
				${GRPC_SRCS})
				
target_include_directories(rvision PRIVATE ${SRC_DIR})
target_include_directories(rvision PRIVATE ${RPC_DIR}/server ${RPC_DIR}/messages)
target_link_libraries(rvision PRIVATE Poco::Util Poco::Net spdlog sqlite3 grpc ${CMAKE_THREAD_LIBS_INIT} ${GRPCPP_LIBRARIES} ${Protobuf_LIBRARIES})

install(TARGETS rvision)
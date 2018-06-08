cmake_minimum_required(VERSION 2.8)
project(reportserver)
find_package(gflags REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11 -DDPOSITION -DMULTIPLE_THREAD -DDEBUG")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/../comm-lib/xflagger/include)
include_directories(${PROJECT_SOURCE_DIR}/../comm-lib/db/include/)
include_directories(${PROJECT_SOURCE_DIR}/../comm-lib/config/tnode/include)
include_directories(${PROJECT_SOURCE_DIR}/../comm-lib/tnode2.0.1/include)
include_directories(${PROJECT_SOURCE_DIR}/../comm-lib/xlogger/include)

set(SOURCE_FILE main.cc 
	src/define.cc
	src/xdatetime.cc
        src/tnode_adapter.cc
        )

link_directories(${PROJECT_SOURCE_DIR}/../comm-lib/market/lib)
link_directories(${PROJECT_SOURCE_DIR}/../comm-lib/tnode2.0.1/lib)
link_directories(${PROJECT_SOURCE_DIR}/../comm-lib/xlogger/lib)
link_directories(${PROJECT_SOURCE_DIR}/../comm-lib/db/lib)
link_directories(${PROJECT_SOURCE_DIR}/../comm-lib/xflagger/lib)

add_executable(reportserver ${SOURCE_FILE})

target_link_libraries(reportserver tnode2 rabbitmq ev xlogger solution_config xconfig mysqlop redisop gflags glog pthread mysqlclient hiredis uv uuid)


cmake_minimum_required(VERSION 3.9)
project(tcp_client)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(source_supervisor_dir ${CMAKE_CURRENT_SOURCE_DIR}/src_supervisor)
#set(source_node_dir ${CMAKE_CURRENT_SOURCE_DIR}/src_node)
set(source_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(header_ext .h)
set(source_ext .cpp)

# Targets
#set(target_supervisor supervisor)
set(target rwell)

# Supervisor target
file(GLOB_RECURSE sources ${source_dir}/*${source_ext})
file(GLOB_RECURSE headers ${source_dir}/*${header_ext})
add_executable(${target} main${source_ext} ${sources})
target_link_libraries(${target} rt m pthread)

# Node target
#file(GLOB_RECURSE sources_node ${source_node_dir}/*${source_ext})
#file(GLOB_RECURSE headers_node ${source_node_dir}/*${header_ext})
#add_executable(${target_node} node${source_ext} ${sources_node})
#target_link_libraries(${target_node} m open62541 pthread)


# Set headers
foreach(_header ${headers})
    get_filename_component(_include_dir ${_header} DIRECTORY)
    target_include_directories(${target} PRIVATE ${_include_dir})
endforeach()

#foreach(_header ${headers_node})
#    get_filename_component(_include_dir ${_header} DIRECTORY)
#    target_include_directories(${target_node} PRIVATE ${_include_dir})
#endforeach()

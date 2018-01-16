cmake_minimum_required (VERSION 2.8)

project (CMimircache)
set (PROJECT_WEB "http://mimircache.info")
set (CMimircache_VERSION_MAJOR 0)
set (CMimircache_VERSION_MINOR 1)
set (CMimircache_VERSION_REVISION 0)



# add_compile_options(-std=c++11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")


include_directories(headers)
include_directories(dataStructure/include)
include_directories(cache/include)
include_directories(cacheReader/include)
include_directories(profiler/include)
include_directories(utils/include)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")



SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")


# find libglib2.0-dev
find_package(GLib "2.40" REQUIRED)
include_directories(${GLib_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLib_LIBRARY})


message(STATUS "glib found? " ${GLib_FOUND})
message(STATUS "LIBS = " ${LIBS})
message(STATUS "glib header = " ${GLib_INCLUDE_DIRS})

aux_source_directory(. dirCMimircache)
message(STATUS "CMimircache sources = " ${dirCMimircache})


add_subdirectory(cache)
add_subdirectory(cacheReader)
add_subdirectory(dataStructure)
add_subdirectory(profiler)
add_subdirectory(utils)


add_executable(CMimircache ${dirCMimircache})

target_link_libraries(CMimircache cache cacheReader profiler dataStructure utils m ${LIBS} ${CMAKE_THREAD_LIBS_INIT} -lpthread)





# Example Usages
# mkdir build
# cd    build
# rm -r *; cmake .. -G Ninja; ninja
# cmake ..; make -j12


# cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake -DCMAKE_BUILD_TYPE=Release ..
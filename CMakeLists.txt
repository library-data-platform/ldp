#
# To build a release version:
#
#     mkdir -p build
#     cd build
#     cmake ..
#     make
#
# For debugging:
#
#     cmake -DDEBUG=ON -DOPTIMIZE=OFF ..
#
# For gprof:
#
#     cmake -DGPROF=ON -DDEBUG=ON -DOPTIMIZE=OFF ..
#

cmake_minimum_required (VERSION 3.7.2)
project (LDP)

#SET(CMAKE_VERBOSE_MAKEFILE ON)

set(RapidJSON_DIR "${CMAKE_SOURCE_DIR}/cmake")

#add_subdirectory (src)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall")

IF(CMAKE_COMPILER_IS_GNUCXX)
	#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall")
	set(FSLIB "-lstdc++fs")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

OPTION(OPTIMIZE
	"Build the project with compiler optimization"
	ON)
IF(OPTIMIZE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
ENDIF(OPTIMIZE)

OPTION(DEBUG
	"Build the project in debugging mode"
	OFF)
IF(DEBUG)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og -DDEBUG -DPERF")
ENDIF(DEBUG)

OPTION(GPROF
	"Build the project for gprof (requires DEBUG)"
	OFF)
IF(GPROF)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -DGPROF")
	set(GPROFFLAG "-pg")
ENDIF(GPROF)

#set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIR})
#set(LIBS ${LIBS} ${PostgreSQL_LIBRARY})

#find_package(SQLite3 REQUIRED)

find_package(RapidJSON REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(ldp_obj OBJECT

	etymoncpp/src/curl.cpp
	etymoncpp/src/mallocptr.cpp
	etymoncpp/src/postgres.cpp
	etymoncpp/src/util.cpp
	src/addcolumns.cpp
	src/anonymize.cpp
	src/camelcase.cpp
	src/config.cpp
	src/dbtype.cpp
	src/dbup1.cpp
	src/dropfields.cpp
	src/extract.cpp
	src/init.cpp
	src/initutil.cpp
	src/ldp.cpp
	src/log.cpp
	src/merge.cpp
	src/names.cpp
	src/options.cpp
	src/paging.cpp
	src/schema.cpp
	src/stage.cpp
	src/timer.cpp
	src/update.cpp
	src/users.cpp
	src/util.cpp

	)

add_executable(ldp
	$<TARGET_OBJECTS:ldp_obj>
	src/main.cpp
	)
target_link_libraries(ldp
	${GPROFFLAG}
	${CURL_LIBRARIES}
	${PostgreSQL_LIBRARY}
	#${SQLite3_LIBRARY}
	${FSLIB}
	)

# add_executable(ldp_test
# 	$<TARGET_OBJECTS:ldp_obj>

# 	test/camelcase_test.cpp
# 	test/main_test.cpp

# 	)
# target_link_libraries(ldp_test
# 	${GPROFFLAG}
# 	${CURL_LIBRARIES}
# 	${ODBC_LIBRARY}
# 	${PostgreSQL_LIBRARY}
# 	#${SQLite3_LIBRARY}
# 	${FSLIB}
# 	)

# add_executable(ldp_testint
# 	$<TARGET_OBJECTS:ldp_obj>

# 	testint/main_testint.cpp
# 	testint/server_testint.cpp

# 	)
# target_link_libraries(ldp_testint
# 	${GPROFFLAG}
# 	${CURL_LIBRARIES}
# 	${ODBC_LIBRARY}
# 	${PostgreSQL_LIBRARY}
# 	#${SQLite3_LIBRARY}
# 	${FSLIB}
# 	)

#INSTALL(PROGRAMS ldp DESTINATION /usr/local/bin)

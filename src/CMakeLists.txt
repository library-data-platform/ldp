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

find_package(ODBC REQUIRED)

find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIR})
#set(LIBS ${LIBS} ${PostgreSQL_LIBRARY})

find_package(SQLite3 REQUIRED)

find_package(RapidJSON REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(ldp_obj OBJECT
	
	../etymoncpp/src/curl.cpp
	../etymoncpp/src/mallocptr.cpp
	../etymoncpp/src/odbc.cpp
	../etymoncpp/src/postgres.cpp
	../etymoncpp/src/sqlite3.cpp
	../etymoncpp/src/util.cpp
	anonymize.cpp
	camelcase.cpp
	config.cpp
	dbtype.cpp
	dbup1.cpp
	extract.cpp
	init.cpp
	log.cpp
	merge.cpp
	names.cpp
	options.cpp
	paging.cpp
	schema.cpp
	server.cpp
	stage.cpp
	timer.cpp
	update.cpp
	util.cpp
	
	)

add_executable(ldp
	$<TARGET_OBJECTS:ldp_obj>
	main.cpp
	)
target_link_libraries(ldp
	${GPROFFLAG}
	${CURL_LIBRARIES}
	${ODBC_LIBRARY}
	${PostgreSQL_LIBRARY}
	${SQLite3_LIBRARY}
	${FSLIB}
	)

add_executable(test_ldp
	$<TARGET_OBJECTS:ldp_obj>

	camelcase_test.cpp
	main_test.cpp

	)
target_link_libraries(test_ldp
	${GPROFFLAG}
	${CURL_LIBRARIES}
	${ODBC_LIBRARY}
	${PostgreSQL_LIBRARY}
	${SQLite3_LIBRARY}
	${FSLIB}
	)

add_executable(itest_ldp
	$<TARGET_OBJECTS:ldp_obj>

	main_itest.cpp
	server_itest.cpp

	)
target_link_libraries(itest_ldp
	${GPROFFLAG}
	${CURL_LIBRARIES}
	${ODBC_LIBRARY}
	${PostgreSQL_LIBRARY}
	${SQLite3_LIBRARY}
	${FSLIB}
	)

INSTALL(PROGRAMS ldp DESTINATION /usr/local/bin)


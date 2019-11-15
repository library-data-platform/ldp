SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall")

#IF(CMAKE_COMPILER_IS_GNUCXX)
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pedantic -Wall")
#ENDIF(CMAKE_COMPILER_IS_GNUCXX)

OPTION(DEBUG
    "Build the project in debugging mode"
    OFF)
IF(DEBUG)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DDEBUG")
ENDIF(DEBUG)

OPTION(OPTIMIZE
    "Build the project with compiler optimization"
    ON)
IF(OPTIMIZE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
ENDIF(OPTIMIZE)

#set(CURL_LIBRARY "-lcurl") 
find_package(CURL REQUIRED) 
include_directories(${CURL_INCLUDE_DIR})

find_package(PostgreSQL REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIR})
#set(LIBS ${LIBS} ${PostgreSQL_LIBRARY})

find_package(RapidJSON)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(ldp
    ../etymoncpp/src/postgres.cpp
    ../etymoncpp/src/util.cpp
    camelcase.cpp
    config_json.cpp
    dbtype.cpp
    extract.cpp
    main.cpp
    merge.cpp
    names.cpp
    options.cpp
    paging.cpp
    schema.cpp
    stage_json.cpp
    timer.cpp
    util.cpp
)
target_link_libraries(ldp
    ${CURL_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)

INSTALL(PROGRAMS ldp DESTINATION /usr/local/bin)


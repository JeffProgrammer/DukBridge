cmake_minimum_required(VERSION 2.8)
project(DukBridge)

# Duktape Src
add_library(duktape thirdparty/duktape/src/duktape.h thirdparty/duktape/src/duk_config.h thirdparty/duktape/src/duktape.c)

# Our Src

set(DUKBRIDGE_SRC
	source/dukbridge.cpp
	source/dukbridge.hpp
)
add_library(dukbridge ${DUKBRIDGE_SRC})
target_link_libraries(dukbridge duktape)

include_directories(
	source
	thirdparty/duktape/src
)

# Tests

add_executable(DukBridgeTest tests/main.cpp)
target_link_libraries(DukBridgeTest dukbridge)
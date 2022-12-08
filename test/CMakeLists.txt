# Copyright (c) 2022 Haofan Zheng
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 3.14)

OPTION(
	SIMPLECONCURRENCY_TEST_CXX_STANDARD
	"C++ standard version used to build SimpleConcurrency test executable."
	11
)
OPTION(SIMPLECONCURRENCY_TEST_LCOV "Option to turn on test code coverage." OFF)

################################################################################
# Set compile options
################################################################################

if(MSVC)
	set(COMMON_OPTIONS /W4 /WX /EHsc /MP /GR /Zc:__cplusplus)
	set(DEBUG_OPTIONS /MTd /Od /Zi /DDEBUG)
	set(RELEASE_OPTIONS /MT /Ox /Oi /Ob2 /fp:fast)# /DNDEBUG
else()
	set(COMMON_OPTIONS -pthread -Wall -Wextra -Werror
		-pedantic -Wpedantic -pedantic-errors)
	set(DEBUG_OPTIONS -O0 -g -DDEBUG)
	set(RELEASE_OPTIONS -O2) #-DNDEBUG defined by default
endif()

set(DEBUG_OPTIONS ${COMMON_OPTIONS} ${DEBUG_OPTIONS})
set(RELEASE_OPTIONS ${COMMON_OPTIONS} ${RELEASE_OPTIONS})

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
endif()

################################################################################
# Fetching dependencise
################################################################################

include(FetchContent)

FetchContent_Declare(
	git_simplecmakescripts
	GIT_REPOSITORY https://github.com/zhenghaven/SimpleCMakeScripts.git
	GIT_TAG        v0.3
)
FetchContent_MakeAvailable(git_simplecmakescripts)
simplecmakescripts_enable()

FetchContent_Declare(
	git_googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG        release-1.11.0
)
FetchContent_MakeAvailable(git_googletest)


################################################################################
# Adding testing executable
################################################################################

set(SOURCES_DIR_PATH ${CMAKE_CURRENT_LIST_DIR}/src)

file(GLOB_RECURSE SOURCES ${SOURCES_DIR_PATH}/*.[ch]*)

add_executable(SimpleConcurrency_test ${SOURCES})

target_compile_options(SimpleConcurrency_test
	PRIVATE $<$<CONFIG:>:${DEBUG_OPTIONS}>
			$<$<CONFIG:Debug>:${DEBUG_OPTIONS}>
			$<$<CONFIG:Release>:${RELEASE_OPTIONS}>)
target_link_libraries(SimpleConcurrency_test SimpleConcurrency gtest)

add_test(NAME SimpleConcurrency_test
	COMMAND SimpleConcurrency_test)

set_property(TARGET SimpleConcurrency_test
	PROPERTY CXX_STANDARD ${SIMPLECONCURRENCY_TEST_CXX_STANDARD})

if (SIMPLECONCURRENCY_TEST_LCOV)
	include(SimpleTestCoverage)
	message(STATUS "Setting up test coverage target...")
	simple_cxx_test_coverage(
		SimpleConcurrency_test
		"${PROJECT_SOURCE_DIR}/include"
	)
endif()

cmake_policy(SET CMP0091 NEW)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.conan.txt)
cmake_minimum_required (VERSION 3.15)
project(wsay VERSION 1.4.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GoogleTest)
include(FetchContent)

# General options
set(CMAKE_CXX_STANDARD 17)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)
set(LIB_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

# Output binary to predictable location (fixes cyclic dependency issues).
set(BINARY_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BINARY_OUT_DIR})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BINARY_OUT_DIR})
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BINARY_OUT_DIR})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

# clang-format
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/.clang-format ${CMAKE_CURRENT_BINARY_DIR}/.clang-format COPYONLY)

# Organize unrelated targets to clean IDE hierarchy.
set(DEPENDENCY_FOLDER "Dependencies")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ${DEPENDENCY_FOLDER})


# Compile Options
function(set_compile_options REQUIRED_ARG EXPOSURE)
	if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		target_compile_options(${REQUIRED_ARG} ${EXPOSURE} -Wall -Wextra -Wpedantic -Werror)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		target_compile_options(${REQUIRED_ARG} ${EXPOSURE} -Wall -Wextra -Wpedantic -Werror)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	  # using Intel C++
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		target_compile_definitions(${REQUIRED_ARG} ${EXPOSURE} NOMINMAX UNICODE _UNICODE)
		target_compile_options(${REQUIRED_ARG} ${EXPOSURE} /Zc:__cplusplus /Zc:alignedNew /permissive- /W4 /WX /utf-8)
		set_target_properties(${REQUIRED_ARG} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()
endfunction()


##
# External dependencies.
##
FetchContent_Declare(fea_utils
	GIT_REPOSITORY https://github.com/p-groarke/fea_utils.git
)
FetchContent_MakeAvailable(fea_utils)

FetchContent_Declare(fea_getopt
	GIT_REPOSITORY https://github.com/p-groarke/fea_getopt.git
)
FetchContent_MakeAvailable(fea_getopt)

set(WIL_BUILD_PACKAGING OFF CACHE INTERNAL "")  # Forces the value
set(WIL_BUILD_TESTS OFF CACHE INTERNAL "")  # Forces the value
FetchContent_Declare(wil
	GIT_REPOSITORY git@github.com:p-groarke/wil.git
	GIT_TAG cmake
	# GIT_REPOSITORY https://github.com/microsoft/wil.git
)
FetchContent_MakeAvailable(wil)


##
# libwsay MT
##
set(LIB_NAME lib${PROJECT_NAME})
file(GLOB_RECURSE LIB_HEADERS "libinclude/*.hpp" "libinclude/*.h" "libinclude/*.tpp")
file(GLOB_RECURSE LIB_SOURCES "libsrc/*.cpp" "libsrc/*.c" "libsrc/*.hpp" "libsrc/*.h" "libsrc/*.tpp")
add_library(${LIB_NAME} ${LIB_HEADERS} ${LIB_SOURCES})
target_link_libraries(${LIB_NAME} PUBLIC fea_utils WIL)

# Interface
target_include_directories(${LIB_NAME} PUBLIC
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libinclude>
)
target_include_directories(${LIB_NAME} PRIVATE libsrc) # For based paths.
target_include_directories(${LIB_NAME} PRIVATE  ${LIB_INCLUDE_DIR})
set_compile_options(${LIB_NAME} PUBLIC)

# Library Install Configuration
install(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}_targets)
install(EXPORT ${LIB_NAME}_targets
	NAMESPACE ${LIB_NAME}::
	FILE ${LIB_NAME}-config.cmake
	DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIB_NAME}"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libinclude/wsay" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")


##
# wsay_cmd
##
file(GLOB_RECURSE CMDTOOL_SOURCES "src_cmd/*.cpp" "src_cmd/*.c" "src_cmd/*.hpp" "src_cmd/*.h" "src_cmd/*.tpp")
add_executable(${PROJECT_NAME} ${CMDTOOL_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src_cmd) # For based paths.
target_include_directories(${PROJECT_NAME} PRIVATE  ${LIB_INCLUDE_DIR})
set_compile_options(${PROJECT_NAME} PUBLIC)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_NAME} fea_getopt)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DWSAY_VERSION=L"${PROJECT_VERSION}")
target_compile_definitions(${PROJECT_NAME} PRIVATE -DWIN32_LEAN_AND_MEAN -DWIN32_EXTRA_LEAN -DVC_EXTRALEAN)


##
# wsay_gui
##
find_package(nuklear CONFIG REQUIRED)
find_package(glfw CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
set(GUITOOL_NAME ${PROJECT_NAME}_gui)
file(GLOB_RECURSE GUITOOL_SOURCES "src_gui/*.cpp" "src_gui/*.c" "src_gui/*.hpp" "src_gui/*.h" "src_gui/*.tpp")
add_executable(${GUITOOL_NAME} WIN32 ${GUITOOL_SOURCES})
target_include_directories(${GUITOOL_NAME} PRIVATE src_gui) # For based paths.
target_include_directories(${GUITOOL_NAME} PRIVATE  ${LIB_INCLUDE_DIR})
set_compile_options(${GUITOOL_NAME} PUBLIC)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${GUITOOL_NAME})
target_compile_definitions(${GUITOOL_NAME} PRIVATE -DWSAY_VERSION="${PROJECT_VERSION}")
target_link_libraries(${GUITOOL_NAME} PRIVATE ${LIB_NAME} nuklear::nuklear glfw::glfw glad::glad)



# Tests
option(BUILD_TESTING "Build and run tests." Off)
if (${BUILD_TESTING})
	enable_testing()
	
	set(TEST_NAME ${PROJECT_NAME}_tests)
	file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp" "tests/*.c" "tests/*.hpp" "tests/*.h" "tests/*.tpp")
	add_executable(${TEST_NAME} ${TEST_SOURCES})

	find_package(GTest CONFIG REQUIRED)
	target_include_directories(${TEST_NAME} PRIVATE  ${LIB_INCLUDE_DIR})
	target_link_libraries(${TEST_NAME} PRIVATE ${LIB_NAME} GTest::GTest)

	gtest_discover_tests(${TEST_NAME})
	add_dependencies(${TEST_NAME} ${PROJECT_NAME})

	set_compile_options(${TEST_NAME} PUBLIC)

	##
	# Copy test data on build.
	##
	set(DATA_IN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)
	set(DATA_OUT_DIR ${BINARY_OUT_DIR}/tests_data)
	add_custom_command(TARGET ${TEST_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${DATA_OUT_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${DATA_IN_DIR} ${DATA_OUT_DIR}
	)
endif()
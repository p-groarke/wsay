cmake_policy(SET CMP0091 NEW)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.conan.txt)
cmake_minimum_required (VERSION 3.10)
project(wsay VERSION 1.2.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GoogleTest)
include(ExternalProject)

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

# Static MSVC runtimes
set(CompilerFlags
	CMAKE_CXX_FLAGS
	CMAKE_CXX_FLAGS_DEBUG
	CMAKE_CXX_FLAGS_RELEASE
	CMAKE_C_FLAGS
	CMAKE_C_FLAGS_DEBUG
	CMAKE_C_FLAGS_RELEASE
)
foreach(CompilerFlag ${CompilerFlags})
	string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

# Compile Options
function(set_compile_options REQUIRED_ARG)
	if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		target_compile_options(${REQUIRED_ARG} PRIVATE -Wall -Wextra -Werror)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		target_compile_options(${REQUIRED_ARG} PRIVATE -Wall -Wextra -Werror)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	  # using Intel C++
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		target_compile_definitions(${REQUIRED_ARG} PUBLIC NOMINMAX)
		target_compile_options(${REQUIRED_ARG} PRIVATE /Zc:__cplusplus /Zc:alignedNew /permissive- /W4 /WX)
	endif()
endfunction()


##
# libwsay
##
set(LIB_NAME lib${PROJECT_NAME})
file(GLOB_RECURSE LIB_HEADERS "libinclude/*.hpp" "libinclude/*.h" "libinclude/*.tpp")
file(GLOB_RECURSE LIB_SOURCES "libsrc/*.cpp" "libsrc/*.c" "libsrc/*.hpp" "libsrc/*.h" "libsrc/*.tpp")
add_library(${LIB_NAME} ${LIB_HEADERS} ${LIB_SOURCES})
# Interface
target_include_directories(${LIB_NAME} PUBLIC
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libinclude>
)
target_include_directories(${LIB_NAME} PRIVATE libsrc) # For based paths.
target_include_directories(${LIB_NAME} PRIVATE  ${LIB_INCLUDE_DIR})
set_compile_options(${LIB_NAME})

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
set_compile_options(${PROJECT_NAME})
add_definitions(-DWSAY_VERSION="${PROJECT_VERSION}")
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_NAME})


##
# wsay_gui
##
find_package(nuklear CONFIG REQUIRED)
set(GUITOOL_NAME ${PROJECT_NAME}_gui)
file(GLOB_RECURSE GUITOOL_SOURCES "src_gui/*.cpp" "src_gui/*.c" "src_gui/*.hpp" "src_gui/*.h" "src_gui/*.tpp")
add_executable(${GUITOOL_NAME} ${GUITOOL_SOURCES})
target_include_directories(${GUITOOL_NAME} PRIVATE src_gui) # For based paths.
target_include_directories(${GUITOOL_NAME} PRIVATE  ${LIB_INCLUDE_DIR})
set_compile_options(${GUITOOL_NAME})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${GUITOOL_NAME})
add_definitions(-DWSAY_VERSION="${PROJECT_VERSION}")
target_link_libraries(${GUITOOL_NAME} PRIVATE ${LIB_NAME} nuklear::nuklear)


##
# External dependencies.
##
ExternalProject_Add(ns_getopt_ext
	GIT_REPOSITORY https://github.com/p-groarke/ns_getopt.git
	CMAKE_ARGS #-DCMAKE_BUILD_TYPE=Release #${CMAKE_ARGS}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
		-DCMAKE_PREFIX_PATH=${CMAKE_CURRENT_BINARY_DIR}
		-DBUILD_EXAMPLES=Off
		-DBUILD_TESTING=Off
	UPDATE_COMMAND ""
)

ExternalProject_Add(wil_ext
	GIT_REPOSITORY https://github.com/microsoft/wil.git
	CONFIGURE_COMMAND ""
	BUILD_COMMAND
		COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include
				${LIB_INCLUDE_DIR}
	INSTALL_COMMAND ""
	UPDATE_COMMAND ""
)

ExternalProject_Add(fea_utils_ext
	GIT_REPOSITORY https://github.com/p-groarke/fea_utils.git
	CMAKE_ARGS #-DCMAKE_BUILD_TYPE=Release #${CMAKE_ARGS}
		-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
		-DCMAKE_PREFIX_PATH=${CMAKE_CURRENT_BINARY_DIR}
		-DBUILD_TESTING=Off
	UPDATE_COMMAND ""
)

set_target_properties(ns_getopt_ext wil_ext fea_utils_ext PROPERTIES FOLDER ${DEPENDENCY_FOLDER})
add_dependencies(${PROJECT_NAME} ns_getopt_ext fea_utils_ext)
add_dependencies(${GUITOOL_NAME} fea_utils_ext)
add_dependencies(${LIB_NAME} wil_ext fea_utils_ext)
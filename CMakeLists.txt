cmake_policy(SET CMP0091 NEW)
cmake_minimum_required (VERSION 3.15)
project(wsay VERSION 1.6.2 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GoogleTest)
include(FetchContent)

# Build options.
option(WSAY_TESTS "Build and run tests." On)

# Get our fea_cmake helpers.
if (${FEA_CMAKE_LOCAL})
	set(FETCHCONTENT_SOURCE_DIR_FEA_CMAKE ${CMAKE_CURRENT_SOURCE_DIR}/../fea_cmake CACHE INTERNAL "")
endif()
FetchContent_Declare(fea_cmake
	GIT_REPOSITORY https://github.com/p-groarke/fea_cmake.git
	GIT_TAG main
)
FetchContent_MakeAvailable(fea_cmake)
include(${fea_cmake_SOURCE_DIR}/CMakeLists.txt)

# Use standard fea_cmake settings.
fea_standard_settings()

# Use fea clang-format file.
fea_import_clangformat()

# Use fea gitignore file.
fea_import_gitignore()

# Output to predictable location.
fea_set_output_dir(${CMAKE_BINARY_DIR}/bin)

# General options
set(CMAKE_CXX_STANDARD 20)


# Set fea_libs options.
if (${FEA_LIBS_LOCAL})
	set(FEA_TESTS On CACHE INTERNAL "")  # Forces the value
else()
	set(FEA_TESTS Off CACHE INTERNAL "")
endif()
set(FEA_CPP20 On CACHE INTERNAL "")
set(FEA_WITH_TBB Off CACHE INTERNAL "")

# Pull fea_libs.
fea_fetch_content(fea_libs
	https://github.com/p-groarke/fea_libs.git
	main
)

# Set wil options.
set(WIL_BUILD_PACKAGING OFF CACHE INTERNAL "")
set(WIL_BUILD_TESTS OFF CACHE INTERNAL "")

# Fetch wil
fea_fetch_content(wil
	https://github.com/microsoft/wil.git
	master
)

# Pull conan
# fea_pull_conan_debug()
# fea_pull_conan_release()
fea_pull_conan_debug(CONAN_ARGS "-s compiler.runtime=MTd --build missing")
fea_pull_conan_release(CONAN_ARGS "-s compiler.runtime=MT --build missing")

# libwsay
set(LIB_NAME lib${PROJECT_NAME})
file(GLOB_RECURSE LIB_HEADERS "libinclude/*.hpp" "libinclude/*.h" "libinclude/*.tpp")
file(GLOB_RECURSE LIB_SOURCES "libsrc/*.cpp" "libsrc/*.c" "libsrc/*.hpp" "libsrc/*.h" "libsrc/*.tpp")
add_library(${LIB_NAME} ${LIB_HEADERS} ${LIB_SOURCES})
target_include_directories(${LIB_NAME} PRIVATE libsrc) # For based paths.

fea_set_compile_options(${LIB_NAME} PUBLIC)
fea_static_runtime(${LIB_NAME})
fea_whole_program_optimization(${LIB_NAME} PUBLIC)

target_link_libraries(${LIB_NAME} PUBLIC fea_libs WIL)

# Interface
target_include_directories(${LIB_NAME} PUBLIC
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libinclude>
)

# Library Install Configuration
install(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}_targets)
install(EXPORT ${LIB_NAME}_targets
	NAMESPACE ${LIB_NAME}::
	FILE ${LIB_NAME}-config.cmake
	DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIB_NAME}"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libinclude/wsay" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")


# wsay
file(GLOB_RECURSE CMDTOOL_SOURCES
		"src/*.cpp" "src/*.c" "src/*.hpp" "src/*.h" "src/*.tpp"
)
add_executable(${PROJECT_NAME} ${CMDTOOL_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src) # For based paths.

fea_set_compile_options(${PROJECT_NAME} PUBLIC)
fea_static_runtime(${PROJECT_NAME})
fea_whole_program_optimization(${PROJECT_NAME} PUBLIC)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_NAME} fea_libs)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DWSAY_VERSION=L"${PROJECT_VERSION}")
target_compile_definitions(${PROJECT_NAME} PRIVATE -DWIN32_LEAN_AND_MEAN -DWIN32_EXTRA_LEAN -DVC_EXTRALEAN)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "\"test <silence msec=\\\"500\\\" /> test\"")

# Tests
if (WSAY_TESTS)
	# enable_testing()

	find_package(GTest CONFIG REQUIRED)

	set(TEST_NAME ${PROJECT_NAME}_tests)
	file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp" "tests/*.c" "tests/*.hpp" "tests/*.h" "tests/*.tpp")
	add_executable(${TEST_NAME} ${TEST_SOURCES})
	target_link_libraries(${TEST_NAME} PRIVATE ${LIB_NAME} GTest::GTest)

	# gtest_discover_tests(${TEST_NAME})
	add_dependencies(${TEST_NAME} ${PROJECT_NAME})

	fea_set_compile_options(${TEST_NAME} PUBLIC)
	fea_static_runtime(${TEST_NAME})
	fea_whole_program_optimization(${TEST_NAME} PUBLIC)
	# set_target_properties(${TEST_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	# Copy test data on build.
	set(DATA_IN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)
	set(DATA_OUT_DIR ${BINARY_OUT_DIR}/tests_data)
	add_custom_command(TARGET ${TEST_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${DATA_OUT_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${DATA_IN_DIR} ${DATA_OUT_DIR}
	)
endif()
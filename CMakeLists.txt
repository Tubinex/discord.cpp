cmake_minimum_required(VERSION 3.20)

project(discordcpp VERSION 0.1.0 LANGUAGES CXX)

option(DISCORDCPP_BUILD_EXAMPLES "Build discordcpp examples" ON)
option(DISCORDCPP_BUILD_TESTS "Build discordcpp tests" ON)
option(DISCORDCPP_ENABLE_WARNINGS "Enable extra compiler warnings for discordcpp" ON)
option(DISCORDCPP_ENABLE_CLANG_TIDY "Enable clang-tidy for discordcpp target" OFF)

if(NOT DEFINED CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
include(CMakePackageConfigHelpers)

find_program(DISCORDCPP_CLANG_TIDY_EXE NAMES clang-tidy)
find_program(DISCORDCPP_CLANG_FORMAT_EXE NAMES clang-format)

set(BUILD_TESTING OFF CACHE BOOL "Disable dependency tests" FORCE)

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
  set(JSON_Install OFF CACHE BOOL "" FORCE)
  set(JSON_MultipleHeaders OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(IXWebSocket QUIET)
if(NOT IXWebSocket_FOUND)
  set(IXWEBSOCKET_BUILD_TEST OFF CACHE BOOL "" FORCE)
  set(IXWEBSOCKET_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
  set(USE_TLS ON CACHE BOOL "" FORCE)
  set(USE_OPEN_SSL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.5
  )
  FetchContent_MakeAvailable(ixwebsocket)
endif()

find_package(cpr QUIET)
if(NOT cpr_FOUND)
  set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)
  set(CPR_ENABLE_SSL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.11.2
  )
  FetchContent_MakeAvailable(cpr)
endif()

find_package(Threads REQUIRED)

if(TARGET ixwebsocket::ixwebsocket)
  set(DISCORDCPP_IXWEBSOCKET_TARGET ixwebsocket::ixwebsocket)
elseif(TARGET IXWebSocket::IXWebSocket)
  set(DISCORDCPP_IXWEBSOCKET_TARGET IXWebSocket::IXWebSocket)
elseif(TARGET ixwebsocket)
  set(DISCORDCPP_IXWEBSOCKET_TARGET ixwebsocket)
else()
  message(FATAL_ERROR "ixwebsocket target not found")
endif()

if(TARGET cpr::cpr)
  set(DISCORDCPP_CPR_TARGET cpr::cpr)
elseif(TARGET cpr)
  set(DISCORDCPP_CPR_TARGET cpr)
else()
  message(FATAL_ERROR "cpr target not found")
endif()

file(GLOB_RECURSE DISCORDCPP_SOURCES CONFIGURE_DEPENDS
  ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_library(discordcpp ${DISCORDCPP_SOURCES})

add_library(discordcpp::discordcpp ALIAS discordcpp)

target_compile_features(discordcpp PUBLIC cxx_std_17)

target_include_directories(discordcpp
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(discordcpp
  PUBLIC
    $<BUILD_INTERFACE:${DISCORDCPP_IXWEBSOCKET_TARGET}>
    $<BUILD_INTERFACE:${DISCORDCPP_CPR_TARGET}>
    $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
    $<INSTALL_INTERFACE:IXWebSocket::IXWebSocket>
    $<INSTALL_INTERFACE:cpr::cpr>
    $<INSTALL_INTERFACE:nlohmann_json::nlohmann_json>
  PRIVATE
    Threads::Threads
)

if(DISCORDCPP_ENABLE_WARNINGS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(discordcpp PRIVATE -Wall -Wextra -Wpedantic)
  elseif(MSVC)
    target_compile_options(discordcpp PRIVATE /W4)
  endif()
endif()

if(DISCORDCPP_ENABLE_CLANG_TIDY)
  if(DISCORDCPP_CLANG_TIDY_EXE)
    set_target_properties(discordcpp PROPERTIES CXX_CLANG_TIDY "${DISCORDCPP_CLANG_TIDY_EXE}")
  else()
    message(WARNING "DISCORDCPP_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found")
  endif()
endif()

file(GLOB_RECURSE DISCORDCPP_FORMAT_FILES CONFIGURE_DEPENDS
  ${PROJECT_SOURCE_DIR}/include/*.h
  ${PROJECT_SOURCE_DIR}/include/*.hpp
  ${PROJECT_SOURCE_DIR}/src/*.h
  ${PROJECT_SOURCE_DIR}/src/*.hpp
  ${PROJECT_SOURCE_DIR}/src/*.c
  ${PROJECT_SOURCE_DIR}/src/*.cc
  ${PROJECT_SOURCE_DIR}/src/*.cpp
  ${PROJECT_SOURCE_DIR}/tests/*.h
  ${PROJECT_SOURCE_DIR}/tests/*.hpp
  ${PROJECT_SOURCE_DIR}/tests/*.c
  ${PROJECT_SOURCE_DIR}/tests/*.cc
  ${PROJECT_SOURCE_DIR}/tests/*.cpp
  ${PROJECT_SOURCE_DIR}/examples/*.h
  ${PROJECT_SOURCE_DIR}/examples/*.hpp
  ${PROJECT_SOURCE_DIR}/examples/*.c
  ${PROJECT_SOURCE_DIR}/examples/*.cc
  ${PROJECT_SOURCE_DIR}/examples/*.cpp
)

if(DISCORDCPP_CLANG_FORMAT_EXE)
  add_custom_target(format
    COMMAND ${DISCORDCPP_CLANG_FORMAT_EXE} -i ${DISCORDCPP_FORMAT_FILES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Formatting project sources with clang-format"
    VERBATIM
  )

  add_custom_target(format-check
    COMMAND ${DISCORDCPP_CLANG_FORMAT_EXE} --dry-run --Werror ${DISCORDCPP_FORMAT_FILES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Checking project formatting with clang-format"
    VERBATIM
  )
else()
  add_custom_target(format
    COMMAND ${CMAKE_COMMAND} -E echo "clang-format was not found in PATH"
    COMMAND ${CMAKE_COMMAND} -E false
  )

  add_custom_target(format-check
    COMMAND ${CMAKE_COMMAND} -E echo "clang-format was not found in PATH"
    COMMAND ${CMAKE_COMMAND} -E false
  )
endif()

set(DISCORDCPP_LINT_FILES
  ${PROJECT_SOURCE_DIR}/src/gateway/Gateway.cpp
  ${PROJECT_SOURCE_DIR}/src/events/Dispatcher.cpp
  ${PROJECT_SOURCE_DIR}/src/http/Client.cpp
)

if(DISCORDCPP_BUILD_EXAMPLES)
  list(APPEND DISCORDCPP_LINT_FILES ${PROJECT_SOURCE_DIR}/examples/BasicConsumer.cpp)
endif()

if(DISCORDCPP_BUILD_TESTS)
  list(APPEND DISCORDCPP_LINT_FILES ${PROJECT_SOURCE_DIR}/tests/EventDispatcherTests.cpp)
endif()

if(DISCORDCPP_CLANG_TIDY_EXE)
  add_custom_target(lint
    COMMAND ${DISCORDCPP_CLANG_TIDY_EXE}
            --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy
            --quiet
            -p=${CMAKE_BINARY_DIR}
            ${DISCORDCPP_LINT_FILES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running clang-tidy"
    VERBATIM
  )
else()
  add_custom_target(lint
    COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy was not found in PATH"
    COMMAND ${CMAKE_COMMAND} -E false
  )
endif()

if(DISCORDCPP_BUILD_EXAMPLES)
  add_executable(discordcpp_basic_consumer examples/BasicConsumer.cpp)
  target_link_libraries(discordcpp_basic_consumer PRIVATE discordcpp::discordcpp)
endif()

if(DISCORDCPP_BUILD_TESTS)
  enable_testing()
  add_executable(discordcpp_event_dispatcher_tests tests/EventDispatcherTests.cpp)
  target_link_libraries(discordcpp_event_dispatcher_tests PRIVATE discordcpp::discordcpp Threads::Threads)
  add_test(NAME discordcpp_event_dispatcher_tests COMMAND discordcpp_event_dispatcher_tests)
endif()

install(TARGETS discordcpp
  EXPORT discordcppTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(EXPORT discordcppTargets
  FILE discordcppTargets.cmake
  NAMESPACE discordcpp::
  DESTINATION lib/cmake/discordcpp
)

configure_package_config_file(
  cmake/discordcppConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/discordcppConfig.cmake
  INSTALL_DESTINATION lib/cmake/discordcpp
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/discordcppConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/discordcppConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/discordcppConfigVersion.cmake
  DESTINATION lib/cmake/discordcpp
)

install(DIRECTORY include/ DESTINATION include)

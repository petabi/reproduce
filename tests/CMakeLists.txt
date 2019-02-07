include(GoogleTest)

set(CMAKE_PROGRAM_PATH /usr/pkg /opt/pkg /usr/local /usr /)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

add_executable(test_config t_config.cpp)
target_include_directories(test_config PRIVATE ${PROJECT_BINARY_DIR})
target_compile_options(test_config PRIVATE -Wno-global-constructors)
if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
  target_compile_options(test_config PUBLIC
    -Wno-zero-as-null-pointer-constant)
endif()
target_link_libraries(test_config libreproduce GTest::GTest GTest::Main)
gtest_discover_tests(test_config)


add_executable(test_matcher t_matcher.cpp)
target_include_directories(test_matcher PRIVATE ${PROJECT_BINARY_DIR})
target_compile_options(test_matcher PRIVATE -Wno-global-constructors)
if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
  target_compile_options(test_matcher PUBLIC
    -Wno-zero-as-null-pointer-constant)
endif()
target_link_libraries(test_matcher libreproduce GTest::GTest GTest::Main)
gtest_discover_tests(test_matcher)

add_executable(test_forward_proto t_forward_proto.cpp)
target_include_directories(test_forward_proto PRIVATE ${PROJECT_BINARY_DIR})
target_compile_options(test_forward_proto PRIVATE -Wno-global-constructors)
if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
  target_compile_options(test_forward_proto PUBLIC
    -Wno-zero-as-null-pointer-constant)
endif()
target_link_libraries(test_forward_proto libreproduce GTest::GTest GTest::Main)
gtest_discover_tests(test_forward_proto)

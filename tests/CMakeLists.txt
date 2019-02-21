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

add_executable(test_converter t_converter.cpp)
target_include_directories(test_converter PRIVATE ${PROJECT_BINARY_DIR})
target_compile_options(test_converter PRIVATE -Wno-global-constructors)
if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
  target_compile_options(test_converter PUBLIC
    -Wno-zero-as-null-pointer-constant)
endif()
target_link_libraries(test_converter libreproduce GTest::GTest GTest::Main)
gtest_discover_tests(test_converter)

add_executable(test_producer t_producer.cpp)
target_include_directories(test_producer PRIVATE ${PROJECT_BINARY_DIR})
target_compile_options(test_producer PRIVATE -Wno-global-constructors)
if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
  target_compile_options(test_producer PUBLIC
    -Wno-zero-as-null-pointer-constant)
endif()
target_link_libraries(test_producer libreproduce GTest::GTest GTest::Main)
gtest_discover_tests(test_producer)

add_executable(test_entropy t_entropy.cpp)
target_include_directories(test_entropy PRIVATE ${PROJECT_BINARY_DIR})
target_compile_options(test_entropy PRIVATE -Wno-global-constructors)
if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
  target_compile_options(test_entropy PUBLIC
    -Wno-zero-as-null-pointer-constant)
endif()
target_link_libraries(test_entropy libreproduce GTest::GTest GTest::Main)
gtest_discover_tests(test_entropy)

add_executable(test_session t_session.cpp)
target_include_directories(test_session PRIVATE ${PROJECT_BINARY_DIR})
target_compile_options(test_session PRIVATE -Wno-global-constructors)
if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
  target_compile_options(test_session PUBLIC
    -Wno-zero-as-null-pointer-constant)
endif()
target_link_libraries(test_session libreproduce GTest::GTest GTest::Main)
gtest_discover_tests(test_session)

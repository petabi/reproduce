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

#add_executable(test_converter t_converter.cpp)
#target_include_directories(test_converter PRIVATE ${PROJECT_BINARY_DIR})
#target_compile_options(test_converter PRIVATE -Wno-global-constructors)
#if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
#  target_compile_options(test_converter PUBLIC
#    -Wno-zero-as-null-pointer-constant)
#endif()
#target_link_libraries(test_converter libreproduce GTest::GTest GTest::Main)
#gtest_discover_tests(test_converter)

add_executable(test_producer t_producer.cpp)
target_include_directories(test_producer PRIVATE ${PROJECT_BINARY_DIR})
target_compile_options(test_producer PRIVATE -Wno-global-constructors)
if(HAVE_CXX_NO_ZERO_AS_NULL_POINTER_CONSTANT)
  target_compile_options(test_producer PUBLIC
    -Wno-zero-as-null-pointer-constant)
endif()
target_link_libraries(test_producer libreproduce GTest::GTest GTest::Main)
gtest_discover_tests(test_producer)

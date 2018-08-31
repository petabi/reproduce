set(PROGRAM_NAME packetproducer)

find_path(LIBRDKAFKA_INCLUDE_DIR librdkafka/rdkafkacpp.h)
find_library(LIBRDKAFKA_LIB rdkafka++)
add_executable(${PROGRAM_NAME} main.cpp controller.cpp converter.cpp producer.cpp options.cpp config.cpp report.cpp)
target_include_directories(${PROGRAM_NAME} PRIVATE ${LIBRDKAFKA_INCLUDE_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/ext)
target_link_libraries(${PROGRAM_NAME} PRIVATE ${LIBRDKAFKA_LIB})
#install(TARGETS ${PROGRAM_NAME} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR})

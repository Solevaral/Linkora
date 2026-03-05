add_executable(test_crypto test_crypto.cpp)
target_include_directories(test_crypto PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_crypto PRIVATE linkora_core)

add_test(NAME linkora_test_crypto COMMAND test_crypto)

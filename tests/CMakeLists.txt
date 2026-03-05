add_executable(test_crypto test_crypto.cpp)
target_include_directories(test_crypto PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_crypto PRIVATE linkora_core)

add_test(NAME linkora_test_crypto COMMAND test_crypto)

add_executable(test_password test_password.cpp)
target_include_directories(test_password PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_password PRIVATE linkora_core)

add_test(NAME linkora_test_password COMMAND test_password)

add_executable(test_frame test_frame.cpp ${PROJECT_SOURCE_DIR}/src/network/frame.cpp)
target_include_directories(test_frame PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_frame PRIVATE linkora_core)

add_test(NAME linkora_test_frame COMMAND test_frame)

add_executable(test_ubx test_ubx.c)
add_executable(test_ubx_msg test_ubx_msg.c)

target_link_libraries(test_ubx PRIVATE ubx)
target_link_libraries(test_ubx_msg PRIVATE ubx)

if(MSVC)
    target_compile_options(test_ubx PRIVATE /W4)
    target_compile_options(test_ubx_msg PRIVATE /W4)
else()
    target_compile_options(test_ubx PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(test_ubx_msg PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ubx_core COMMAND test_ubx)
add_test(NAME ubx_msg COMMAND test_ubx_msg)

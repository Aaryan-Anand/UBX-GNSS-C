add_executable(test_ubx test_ubx.c)

target_link_libraries(test_ubx PRIVATE ubx)

if(MSVC)
    target_compile_options(test_ubx PRIVATE /W4)
else()
    target_compile_options(test_ubx PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME ubx_core COMMAND test_ubx)

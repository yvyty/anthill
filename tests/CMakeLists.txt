add_executable(test_args test_args.c)
target_link_libraries(test_args PRIVATE anthill_core)

add_executable(test_port_set test_port_set.c)
target_link_libraries(test_port_set PRIVATE anthill_core)

add_executable(test_scan test_scan.c)
target_link_libraries(test_scan PRIVATE anthill_core)

add_executable(test_cli test_cli.c)
target_compile_definitions(
    test_cli
    PRIVATE ANTHILL_BINARY="$<TARGET_FILE:anthill>"
)

foreach(test_target test_args test_port_set test_scan test_cli)
    anthill_apply_warnings(${test_target})
endforeach()

add_test(NAME args COMMAND test_args)
add_test(NAME port_set COMMAND test_port_set)
add_test(NAME scan COMMAND test_scan)
add_test(NAME cli COMMAND test_cli)
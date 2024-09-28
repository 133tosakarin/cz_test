# 假设测试文件名为 test_main.cpp
set(TEST_SOURCES
    test.cc  # 这里添加你的测试文件
)


message("TEST_SOURCES: ${TEST_SOURCES}")

# 创建可执行文件
add_executable(run_tests ${TEST_SOURCES})
add_executable(run_test2 tt.cc)

# 链接库
target_link_libraries(run_tests concurrency)


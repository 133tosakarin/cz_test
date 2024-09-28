
message("current_dir: ${CMAKE_SOURCE_DIR}")

# 定义源文件
set(SOURCES

  lock_manager.cc
  transaction_manager.cc
)

# 创建库
add_library(concurrency STATIC ${SOURCES})

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:concurrency>
    PARENT_SCOPE)

# 如果需要创建可执行文件，可以使用以下代码
# add_executable(my_executable main.cpp)  # main.cpp 是你的主程序文件
# target_link_libraries(my_executable my_library)

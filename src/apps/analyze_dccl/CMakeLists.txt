add_executable(analyze_dccl analyze_dccl.cpp)
target_link_libraries(analyze_dccl dccl)
install(TARGETS analyze_dccl DESTINATION ${CMAKE_INSTALL_BINDIR})

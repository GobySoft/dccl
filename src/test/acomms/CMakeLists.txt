add_subdirectory(queue1)
add_subdirectory(queue2)
add_subdirectory(queue3)
add_subdirectory(queue4)
add_subdirectory(queue5)
add_subdirectory(queue6)

add_subdirectory(amac1)

add_subdirectory(mmdriver1)
add_subdirectory(mmdriver2)

if(enable_zeromq)
  add_subdirectory(route1)
  add_subdirectory(udpdriver1)
  add_subdirectory(udpdriver2)
endif()

add_subdirectory(bitset1)
file(GLOB SRC common/*.c dumb/*.c)

add_executable(dfrotz ${SRC})

set_target_properties(dfrotz PROPERTIES UNITY_BUILD OFF)

# target_link_libraries(dfrotz)

install(TARGETS dfrotz RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})


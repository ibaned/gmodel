if(NOT BUILD_TESTING)
  return()
endif()

add_library(minidiff minidiff.cpp)
target_link_libraries(minidiff PUBLIC gmodel)
target_include_directories(minidiff INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )

function(test_func TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  target_link_libraries(${TEST_NAME} PRIVATE gmodel minidiff)
  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}_gold.geo
      ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_gold.geo
      COPYONLY)
  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}_gold.dmg
      ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_gold.dmg
      COPYONLY)
  add_test(NAME ${TEST_NAME}_test COMMAND ${TEST_NAME})
endfunction(test_func)

test_func(cube)
test_func(cylinder)
test_func(cube_in_cube)
test_func(spline_shape)

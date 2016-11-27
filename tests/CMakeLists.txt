if(NOT BUILD_TESTING)
  return()
endif()

add_library(minidiff minidiff.cpp)
target_link_libraries(minidiff PUBLIC gmodel)
target_include_directories(minidiff INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  )

function(gold_file BASENAME)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${BASENAME})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${BASENAME}
        ${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}
        COPYONLY)
  else()
    message("warning: ${CMAKE_CURRENT_SOURCE_DIR}/${BASENAME} doesn't exist")
  endif()
endfunction(gold_file)

function(test_func TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  target_link_libraries(${TEST_NAME} PRIVATE gmodel minidiff)
  add_test(NAME ${TEST_NAME}_test COMMAND ${TEST_NAME})
  gold_file(${TEST_NAME}_gold.geo)
  gold_file(${TEST_NAME}_gold.dmg)
endfunction(test_func)

test_func(cube)
test_func(cylinder)
test_func(cube_in_cube)
test_func(spline_shape)
test_func(target)

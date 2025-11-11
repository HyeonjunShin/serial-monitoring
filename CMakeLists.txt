cmake_minimum_required(VERSION 3.20.0)
project(serial_monitoring VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -O0")

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install") 

include(CPack)
set(CPACK_PACKAGE_NAME "serial-monitoring")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "TGZ;ZIP")        
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "hyeonnnjunnn@gmail.com")


find_package(Boost REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(main 
  examples/main.cpp
  ${CMAKE_SOURCE_DIR}/include/realtime_spectrogram_widget.hpp
)
target_include_directories(main PRIVATE 
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(main PRIVATE Qt6::Widgets)


add_executable(gui
  examples/gui.cpp
  ${CMAKE_SOURCE_DIR}/include/realtime_spectrogram_widget.hpp
)
target_include_directories(gui PRIVATE 
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(gui PRIVATE Qt6::Widgets)


if(Boost_FOUND)
  message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
  target_include_directories(main PRIVATE ${Boost_INCLUDE_DIRS})
  # gui 타겟에도 필요하다면 추가
  # target_include_directories(gui PRIVATE ${Boost_INCLUDE_DIRS})
endif()



install(TARGETS main
  RUNTIME DESTINATION bin
)
install(TARGETS gui
  RUNTIME DESTINATION bin
)


install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ 
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(FILES "${CMAKE_SOURCE_DIR}/README.md"
  DESTINATION share/${PROJECT_NAME}
)

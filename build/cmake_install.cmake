# Install script for directory: /home/pi/old/daemon

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  
    if(NOT EXISTS "$ENV{DESTDIR}/etc/pithond.conf")
      #file(INSTALL "/home/pi/old/daemon/./pithond.conf" DESTINATION "/etc")
      message(STATUS "Installing: $ENV{DESTDIR}/etc/pithond.conf")
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy "/home/pi/old/daemon/./pithond.conf"
                      "$ENV{DESTDIR}/etc/pithond.conf"
                      RESULT_VARIABLE copy_result
                      ERROR_VARIABLE error_output)
      if(copy_result)
        message(FATAL_ERROR ${error_output})
      endif()
    else()
      message(STATUS "Skipping  : $ENV{DESTDIR}/etc/pithond.conf")
    endif()
  
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  
    if(NOT EXISTS "$ENV{DESTDIR}/usr/lib/systemd/system//pithond.service")
      #file(INSTALL "/home/pi/old/daemon/./pithond.service" DESTINATION "/usr/lib/systemd/system/")
      message(STATUS "Installing: $ENV{DESTDIR}/usr/lib/systemd/system//pithond.service")
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy "/home/pi/old/daemon/./pithond.service"
                      "$ENV{DESTDIR}/usr/lib/systemd/system//pithond.service"
                      RESULT_VARIABLE copy_result
                      ERROR_VARIABLE error_output)
      if(copy_result)
        message(FATAL_ERROR ${error_output})
      endif()
    else()
      message(STATUS "Skipping  : $ENV{DESTDIR}/usr/lib/systemd/system//pithond.service")
    endif()
  
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/pi/old/daemon/build/src/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/pi/old/daemon/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")

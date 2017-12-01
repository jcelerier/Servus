# Copyright (c) 2015-2016, Human Brain Project
#                          Juan Hernando <jhernando@fi.upm.es>
#
# This file is part of Servus <https://github.com/HBPVIS/Servus>
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License version 3.0 as published
# by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(GenerateExportHeader)
set(SERVUS_PUBLIC_HEADERS
  listener.h
  result.h
  serializable.h
  servus.h
  types.h
  uint128_t.h
  uri.h
  )

set(SERVUS_HEADERS
  avahi/servus.h
  dnssd/servus.h
  none/servus.h
  )

set(SERVUS_SOURCES
  ${SERVUS_PUBLIC_HEADERS}
  ${SERVUS_HEADERS}
  md5/md5.cc
  serializable.cpp
  servus.cpp
  uint128_t.cpp
  uri.cpp
  )

list(APPEND SERVUS_LINK_LIBRARIES PRIVATE ${CMAKE_THREAD_LIBS_INIT})
if(MSVC)
  list(APPEND SERVUS_LINK_LIBRARIES ws2_32)
endif()

if(ANDROID)
  set(SERVUS_SOURCES ${SERVUS_SOURCES}

    android/dnssd_clientlib.c
    android/dnssd_clientstub.c
    android/dnssd_clientshim.c
    android/dnssd_ipc.c
  )
endif()
set(BUILD_SHARED_LIBS OFF)
add_library(Servus_iface INTERFACE)
add_library(Servus ${SERVUS_SOURCES})
generate_export_header(Servus BASE_NAME SERVUS)
target_compile_definitions(Servus PRIVATE Servus_EXPORTS)
target_include_directories(Servus PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(Servus_iface INTERFACE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(ANDROID)
  target_include_directories(Servus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/android)
  target_compile_definitions(Servus PUBLIC SERVUS_USE_DNSSD
      HAVE_IPV6
      NOT_HAVE_SA_LEN
      PLATFORM_NO_RLIMIT
      MDNS_DEBUGMSGS=0
      MDNS_UDS_SERVERPATH=\"/dev/socket/mdnsd\"
      MDNS_USERNAME=\"mdnsr\")
endif()
if(DNSSD_FOUND)
  target_link_libraries(Servus PUBLIC ${DNSSD_LIBRARIES})
  target_compile_definitions(Servus PUBLIC SERVUS_USE_DNSSD)
endif()
if(Avahi_FOUND)
  target_link_libraries(Servus PUBLIC ${Avahi_LIBRARIES})
  target_compile_definitions(Servus PUBLIC SERVUS_USE_AVAHI_CLIENT)
endif()

install(TARGETS Servus
    EXPORT ossia-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)

if(Qt5Core_FOUND)
  add_library(ServusQt qt/itemModel.cpp)
  target_include_directories(ServusQt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
  target_link_libraries(ServusQt Qt5::Core)
endif()

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
  servus.h
  servus_impl.h
  types.h
  )

set(SERVUS_HEADERS
  avahi/servus.h
  dnssd/servus.h
  none/servus.h
  )

set(SERVUS_SOURCES
  ${SERVUS_PUBLIC_HEADERS}
  ${SERVUS_HEADERS}
  )

list(APPEND SERVUS_LINK_LIBRARIES PRIVATE ${CMAKE_THREAD_LIBS_INIT})
if(MSVC)
  list(APPEND SERVUS_LINK_LIBRARIES ws2_32)
endif()

set(BUILD_SHARED_LIBS OFF)
add_library(Servus_iface INTERFACE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
target_include_directories(Servus_iface INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)
if(ANDROID)
  target_include_directories(Servus_iface INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/android>)
  target_compile_definitions(Servus_iface INTERFACE SERVUS_USE_DNSSD
      HAVE_IPV6
      NOT_HAVE_SA_LEN
      PLATFORM_NO_RLIMIT
      MDNS_DEBUGMSGS=0
      MDNS_UDS_SERVERPATH=\"/dev/socket/mdnsd\"
      MDNS_USERNAME=\"mdnsr\")

  add_library(dnssd_android STATIC
    android/dnssd_clientlib.c
    android/dnssd_clientstub.c
    android/dnssd_clientshim.c
    android/dnssd_ipc.c
  )
  target_include_directories(dnssd_android
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/android>
  )
  target_compile_definitions(dnssd_android
    PUBLIC
      HAVE_IPV6
      NOT_HAVE_SA_LEN
      PLATFORM_NO_RLIMIT
      MDNS_DEBUGMSGS=0
      MDNS_UDS_SERVERPATH=\"/dev/socket/mdnsd\"
      MDNS_USERNAME=\"mdnsr\"
  )
  target_link_libraries(Servus_iface INTERFACE dnssd_android)

  export(TARGETS dnssd_android FILE servus-exports.cmake)
  install(TARGETS dnssd_android EXPORT ossia-targets)
endif()

if(TARGET DNSSD::dnssd-static)
  target_link_libraries(Servus_iface INTERFACE DNSSD::dnssd-static)
  target_compile_definitions(Servus_iface INTERFACE SERVUS_USE_DNSSD)
elseif(DNSSD_FOUND)
  target_link_libraries(Servus_iface INTERFACE ${DNSSD_LIBRARIES})
  target_compile_definitions(Servus_iface INTERFACE SERVUS_USE_DNSSD)
endif()

if(Avahi_FOUND)
  target_include_directories(Servus_iface INTERFACE ${Avahi_CLIENT_INCLUDE_DIRS})
  target_link_libraries(Servus_iface INTERFACE ${CMAKE_DL_LIBS})
  target_compile_definitions(Servus_iface INTERFACE SERVUS_USE_AVAHI_CLIENT)
endif()

export(TARGETS Servus_iface FILE servus-exports.cmake)
install(TARGETS Servus_iface EXPORT ossia-targets)

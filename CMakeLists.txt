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

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
cmake_policy(VERSION 3.7)
project(Servus CXX)
if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT WIN32)
  find_package(Avahi)
elseif(APPLE)
  find_package(DNSSD)
elseif(WIN32)
  find_package(DNSSD NO_MODULE)
  
  if(NOT DNSSD_FOUND)
    set(DNSSD_BUILD_SHARED FALSE CACHE "" INTERNAL)
    set(DNSSD_BUILD_STATIC TRUE CACHE "" INTERNAL)
    set(DNSSD_BUILD_EXAMPLES FALSE CACHE "" INTERNAL)
    add_subdirectory(3rdparty/dnssd)
    set(DNSSD_FOUND 1)
    set(DNSSD_LIBRARIES dnssd-static)
  endif()
endif()

if(NOT QT_VERSION)
  set(QT_VERSION Qt6 6.4)
endif()
list(GET QT_VERSION 0 QT_PREFIX)

find_package(${QT_VERSION} QUIET COMPONENTS Core)
find_package(Threads REQUIRED)

if(${QT_PREFIX}Core_FOUND)
  set(SERVUS_DEPENDENT_LIBRARIES ${QT_PREFIX}Core)
endif()

add_subdirectory(servus)


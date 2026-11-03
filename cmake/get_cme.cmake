# SPDX-License-Identifier: AGPL-3.0-only
# cmake-everywhere: one find_package for a library, wherever it comes from.
#
# Set before project(), because a dependency provider may only be installed
# from a file named by CMAKE_PROJECT_TOP_LEVEL_INCLUDES:
#
#   set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/cmake/get_cme.cmake)
#
# A revision and its digest, not a branch. What resolves every dependency of
# this project is a dependency of this project.

# The pin this project carries, and the pin this build was last given.
#
# CME_VERSION is a cache entry, and a cache entry is not overwritten by the
# default beside it: raising the pin here changed nothing in a build
# directory that already had one, silently, and the build went on using the
# revision from before the change. So the pin that was applied is remembered,
# and a pin that differs from it wins -- while -DCME_VERSION= on the command
# line still stands until this file says something else.
set(CME_PINNED "v0.2.53")
set(CME_PINNED_SHA256 "bbbbeb647136ecf338d146e45f7c34d366ca602e40d6eb7656ae5b4fbe43a633")
set(CME_PINNED_URL
  "https://github.com/j4niwzis/cmake-everywhere/releases/download/v0.2.53/cmake-everywhere-0.2.53.tar.gz")
if(NOT "${CME_PIN_APPLIED}" STREQUAL "${CME_PINNED}")
  set(CME_VERSION "${CME_PINNED}" CACHE STRING
    "cmake-everywhere release" FORCE)
  set(CME_SHA256 "${CME_PINNED_SHA256}" CACHE STRING
    "The digest of that release's archive" FORCE)
  set(CME_URL "${CME_PINNED_URL}" CACHE STRING
    "Where that archive is fetched from" FORCE)
  set(CME_PIN_APPLIED "${CME_PINNED}" CACHE INTERNAL
    "The pin this build directory was given")
endif()
set(CME_URL "${CME_PINNED_URL}" CACHE STRING
  "Where that archive is fetched from")
set(CME_SOURCE_DIR "${CMAKE_BINARY_DIR}/_cme" CACHE PATH
  "Where it is unpacked")

# Which revision the directory already holds is not readable from the
# directory, so it is written down beside it. Without that, raising the pin
# changed nothing: the same tree was used, with a number in the log that no
# longer described it.
set(cme_stamp "${CME_SOURCE_DIR}/.cme-revision")
set(cme_have "")
if(EXISTS "${cme_stamp}")
  file(READ "${cme_stamp}" cme_have)
  string(STRIP "${cme_have}" cme_have)
endif()

if(NOT EXISTS "${CME_SOURCE_DIR}/cmake-everywhere.cmake"
   OR NOT cme_have STREQUAL "${CME_VERSION}")
  # An archive somebody else fetched.
  #
  # A build with no network cannot download this. Such a build declares the
  # archive among its own sources -- by the same URL and the same digest --
  # and says where it put it.
  set(CME_ARCHIVE "" CACHE FILEPATH
    "An already-fetched archive of that revision, for a build with no network")
  set(archive "${CMAKE_BINARY_DIR}/cme-${CME_VERSION}.tar.gz")
  if(CME_ARCHIVE)
    if(NOT EXISTS "${CME_ARCHIVE}")
      message(FATAL_ERROR
        "cmake-everywhere: CME_ARCHIVE is ${CME_ARCHIVE}, and there is no "
        "such file")
    endif()
    file(SHA256 "${CME_ARCHIVE}" have)
    if(NOT have STREQUAL CME_SHA256)
      message(FATAL_ERROR
        "cmake-everywhere: ${CME_ARCHIVE} hashes to ${have}, and this project "
        "asks for ${CME_SHA256}. It is not the revision this build was "
        "written against.")
    endif()
    message(STATUS "cmake-everywhere: taking ${CME_VERSION} from ${CME_ARCHIVE}")
    file(COPY_FILE "${CME_ARCHIVE}" "${archive}")
  else()
    if(cme_have)
      message(STATUS
        "cmake-everywhere: fetching ${CME_VERSION}, replacing ${cme_have}")
    else()
      message(STATUS "cmake-everywhere: fetching ${CME_VERSION}")
    endif()
    file(DOWNLOAD "${CME_URL}" "${archive}"
      STATUS status EXPECTED_HASH SHA256=${CME_SHA256})
    list(GET status 0 code)
    if(NOT code EQUAL 0)
      list(GET status 1 reason)
      message(FATAL_ERROR "cmake-everywhere: cannot fetch ${CME_VERSION}: ${reason}")
    endif()
  endif()
  file(ARCHIVE_EXTRACT INPUT "${archive}"
       DESTINATION "${CMAKE_BINARY_DIR}/_cme-unpack")
  file(GLOB unpacked "${CMAKE_BINARY_DIR}/_cme-unpack/*")
  list(GET unpacked 0 root)
  file(REMOVE_RECURSE "${CME_SOURCE_DIR}")
  file(RENAME "${root}" "${CME_SOURCE_DIR}")
  file(WRITE "${cme_stamp}" "${CME_VERSION}\n")
endif()

include("${CME_SOURCE_DIR}/cmake-everywhere.cmake")

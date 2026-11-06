# SPDX-License-Identifier: AGPL-3.0-only
# The providers' own folders, each under src/provider/<name>/ and saying what it
# contributes to the one provider library -- found by that alone, the way mux
# finds its protocols: a new folder is a new provider's parts, with no list at
# the top of the build to edit, and a folder that is not there contributes
# nothing.
file(GLOB pchat_provider_folders LIST_DIRECTORIES true
     RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src/provider"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/provider/*/CMakeLists.txt")
list(TRANSFORM pchat_provider_folders REPLACE "/CMakeLists\\.txt$" "")
foreach(pchat_provider_folder IN LISTS pchat_provider_folders)
  add_subdirectory("src/provider/${pchat_provider_folder}")
endforeach()

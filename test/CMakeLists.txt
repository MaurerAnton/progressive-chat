# SPDX-License-Identifier: AGPL-3.0-only
# The tests: three programs, each its own module, run by ctest. Nothing is
# fetched to say what they say (tests.h), and each is compiled with the same
# std and flags as the modules it reads -- one std BMI in the whole build.
#
#   pchat-core-test    the model: ids, conversations, messages
#   pchat-theme-test   the palette and what the looks give
#   pchat-logic-test   what the screens read out of the model
#   pchat-json-test    what comes in off the wire, read and written back

function(pchat_test name libraries)
  add_executable(pchat-${name}-test)
  target_sources(pchat-${name}-test PRIVATE
    FILE_SET CXX_MODULES BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES ${name}_test.cc)
  target_compile_features(pchat-${name}-test PRIVATE cxx_std_26)
  target_compile_options(pchat-${name}-test PRIVATE ${pchat_std_flags})
  target_link_libraries(pchat-${name}-test PRIVATE ${libraries})
  add_test(NAME pchat-${name}-test COMMAND pchat-${name}-test)
endfunction()

pchat_test(core pchat_core)
pchat_test(theme pchat_core)
pchat_test(logic pchat_core)

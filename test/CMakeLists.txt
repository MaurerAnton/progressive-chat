# SPDX-License-Identifier: AGPL-3.0-only
# The tests: three programs, each its own module, run by ctest. Nothing is
# fetched to say what they say (tests.h), and each is compiled with the same
# std and flags as the modules it reads -- one std BMI in the whole build.
#
#   pchat-core-test    the model: ids, conversations, messages
#   pchat-theme-test   the palette and what the looks give
#   pchat-logic-test   what the screens read out of the model
#   pchat-json-test    what comes off the wire, read into values and written back
#   pchat-provider-test    a body written for a model, and its stream read back
#   pchat-json-test    what comes in off the wire, read and written back

# Every test together, for one build and one ctest: what the check asks for by
# name, so that a test added here is run without adding it there too.
add_custom_target(pchat-tests)

function(pchat_test name libraries)
  add_executable(pchat-${name}-test)
  target_sources(pchat-${name}-test PRIVATE
    FILE_SET CXX_MODULES BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES ${name}_test.cc)
  target_compile_features(pchat-${name}-test PRIVATE cxx_std_26)
  target_compile_options(pchat-${name}-test PRIVATE ${pchat_std_flags})
  target_link_libraries(pchat-${name}-test PRIVATE ${libraries})
  add_test(NAME pchat-${name}-test COMMAND pchat-${name}-test)
  add_dependencies(pchat-tests pchat-${name}-test)
endfunction()

pchat_test(core pchat_core)
pchat_test(theme pchat_core)
pchat_test(logic pchat_core)
pchat_test(json pchat_json)
pchat_test(provider pchat_provider)
pchat_test(provider_run pchat_provider)
pchat_test(provider_loop pchat_provider)
pchat_test(tools pchat_tools)

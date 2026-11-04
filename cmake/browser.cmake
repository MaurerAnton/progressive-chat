# SPDX-License-Identifier: AGPL-3.0-only
# The browser build: a .js and a .wasm that the page loads, and the page
# itself beside them.
#
# Threads, and with them shared memory: skiff-framework's browser libraries
# are linked with -pthread, so a client linked without it is one the linker
# cannot put together. Shared memory is a browser's to give only where the
# page is cross-origin isolated, and a static host says no headers -- so the
# page brings a service worker (web/worker.js) that answers with them.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set_target_properties(progressive-chat PROPERTIES
  OUTPUT_NAME progressive-chat
  SUFFIX ".js")

target_link_options(progressive-chat PRIVATE
  -fexceptions
  -pthread
  -sASYNCIFY
  -sASYNCIFY_STACK_SIZE=2097152
  -sALLOW_MEMORY_GROWTH=1
  -sINITIAL_MEMORY=64MB
  # A megabyte is what a thread that asked for none is given, and a window
  # built of a column of rows does not fit in it: it went before anything was
  # drawn, and what the page said was only that its renderer had gone.
  -sSTACK_SIZE=4MB
  -sPTHREAD_POOL_SIZE=4
  -sENVIRONMENT=web,worker
  -sEXIT_RUNTIME=0
  # A factory the page calls with its options (web/bootstrap.js), rather than
  # a script that reads a global as it loads: what is asked for where the
  # client is linked modularized, and what the page handles either way of.
  -sMODULARIZE=1
  -sEXPORT_NAME=createProgressiveChat
  -sFORCE_FILESYSTEM=1
  -sFETCH=1
  "-sEXPORTED_RUNTIME_METHODS=ccall,UTF8ToString"
  # What the page's own JavaScript calls: the program, and the entry skiff
  # answers its DOM's events through (src/browser_host.cc).
  "-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_skiff_web_event")

# The page: the bootstrap, the worker, and the element skiff writes into,
# beside the client, in build/web -- what the workflow uploads.
set(pchat_web_dir "${CMAKE_CURRENT_BINARY_DIR}/web")
add_custom_command(TARGET progressive-chat POST_BUILD
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/package_web.py"
    --build "${CMAKE_CURRENT_BINARY_DIR}" --output "${pchat_web_dir}"
    --said "${PCHAT_VERSION_SAID}"
  VERBATIM)

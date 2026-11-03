# SPDX-License-Identifier: AGPL-3.0-only
# The browser build: a .wasm with its loader, painted into one canvas.
#
# No threads, and so no cross-origin isolation: SharedArrayBuffer is what
# needs COOP and COEP, and a static host gives neither. skiff is asked for
# SKIFF_EMSCRIPTEN_THREADS OFF above.
set_target_properties(progressive-chat PROPERTIES
  SUFFIX ".html"
  OUTPUT_NAME "progressive-chat")

target_link_options(progressive-chat PRIVATE
  --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/index.html
  -sEXPORT_ES6=1
  -sEXPORT_NAME=createProgressiveChat
  -sALLOW_MEMORY_GROWTH=1
  -sENVIRONMENT=web,worker
  -sFORCE_FILESYSTEM=1
  -sEXIT_RUNTIME=0
  -sSTACK_SIZE=1MB
  -sINITIAL_MEMORY=64MB
  -sFETCH=1)


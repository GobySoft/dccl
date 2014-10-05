file(GLOB doxy_body/* DOXY_SRC)

add_custom_command(OUTPUT ${dccl_SHARE_DIR}/doc/dccl-dev.pdf
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/doxy2pdf.sh
  ARGS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${dccl_SHARE_DIR}/doc/dccl-dev.pdf
  DEPENDS dccl dccl-dev.doxy.in ${DOXY_SRC}
)

add_custom_command(OUTPUT ${dccl_SHARE_DIR}/doc/html/index.html
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/doxy2html.sh
  ARGS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS dccl dccl-dev.doxy.in ${DOXY_SRC}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dccl-dev.doxy.in ${CMAKE_CURRENT_BINARY_DIR}/dccl-dev.doxy @ONLY)

option(build_doc_pdf "Build PDF Developers Documentation" OFF)

if(build_doc_pdf)
  find_package(LATEX REQUIRED)
  add_custom_target(dev-pdf ALL DEPENDS ${dccl_SHARE_DIR}/doc/dccl-dev.pdf)
endif()

option(build_doc_html "Build HTML Developers Documentation" ON)

if(build_doc_html)
  add_custom_target(dev-html ALL DEPENDS ${dccl_SHARE_DIR}/doc/html/index.html)
endif()

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${dccl_SHARE_DIR}/doc/html)

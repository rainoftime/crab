add_executable(pointers tests.cc)
target_link_libraries (pointers ${CRAB_LIBS})

install(TARGETS pointers
  RUNTIME DESTINATION tests/domains
  )


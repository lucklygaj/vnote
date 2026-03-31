target_sources(vnote PRIVATE
  giteesyncconfig.cpp
  giteesyncconfig.h
  giteeapi.cpp
  giteeapi.h
  shacache.cpp
  shacache.h
  giteesyncservice.cpp
  giteesyncservice.h
)

target_include_directories(vnote PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)


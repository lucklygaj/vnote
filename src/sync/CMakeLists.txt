target_sources(vnote PRIVATE
  filesyncstate.h
  giteesyncconfig.cpp
  giteesyncconfig.h
  giteeapi.cpp
  giteeapi.h
  shacache.cpp
  shacache.h
  giteesyncservice.cpp
  giteesyncservice.h
  syncstatusmanager.h
  syncstatusmanager.cpp
  smartsyncscheduler.h
  smartsyncscheduler.cpp
  syncpendinglist.h
  syncpendinglist.cpp
  networkstatusmonitor.h
  networkstatusmonitor.cpp
)

target_include_directories(vnote PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)


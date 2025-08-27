# ~~~
# Author:      bdcat <Dave Register>
# Copyright:
# License:     GPLv2+
# ~~~

# -------- Cmake setup ---------
#

set(OCPN_TEST_REPO
    "david-register/ocpn-plugins-unstable"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_BETA_REPO
    "david-register/ocpn-plugins-unstable"
    CACHE STRING
    "Default repository for tagged builds matching 'beta'"
)
set(OCPN_RELEASE_REPO
    "david-register/ocpn-plugins-stable"
    CACHE STRING
    "Default repository for tagged builds not matching 'beta'"
)

set(OCPN_TARGET_TUPLE "" CACHE STRING
  "Target spec: \"platform;version;arch\""
)

# -------  Plugin setup --------
#
include("VERSION.cmake")
set(PKG_NAME ropeless_pi)
set(PKG_VERSION ${OCPN_VERSION})

set(PKG_PRERELEASE "")  # Empty, or a tag like 'beta'

set(DISPLAY_NAME ropeless)    # Dialogs, installer artifacts, ...
set(PLUGIN_API_NAME ropeless) # As of GetCommonName() in plugin API
set(CPACK_PACKAGE_CONTACT "Dave Register")
set(PKG_SUMMARY "Ropeless Fishing Interface"
)
set(PKG_DESCRIPTION [=[ Ropeless Fishing Plugin]=])

set(PKG_HOMEPAGE https://github.com/bdbcat/ropeless_pi/)
set(PKG_INFO_URL https://www.ropeless.us/)

set(PKG_AUTHOR "Dave register")

SET(SRC_ROPELESS
    src/ropeless_pi.h
    src/ropeless_pi.cpp
    src/Select.h
    src/SelectItem.h
    src/Select.cpp
    src/SelectItem.cpp
    src/vector2d.h
    src/vector2d.cpp
    src/PI_RolloverWin.h
    src/PI_RolloverWin.cpp
    src/TexFont.cpp
    src/TexFont.h
    src/georef.cpp
    src/ODdc.cpp
    src/ODShaders.cpp
    src/icons.cpp
    src/OCPNListCtrl.cpp
    src/pugixml.cpp
    src/mynumdlg.cpp
    src/myokdlg.cpp
    src/manualPlacementDlg.h
    src/manualPlacementDlg.cpp
    src/manualPlacementDlgImpl.h
    src/manualPlacementDlgImpl.cpp
    src/transponderReleaseDlg.h
    src/transponderReleaseDlg.cpp
    src/transponderReleaseDlgImpl.h
    src/transponderReleaseDlgImpl.cpp
    src/haversine.h
    src/haversine.cpp
    src/graphics.h
    src/graphics.cpp
)

SET(SRC_DATA
    data/test_xpdr.png
)

SET(SRC_NMEA0183
    src/nmea0183/LatLong.hpp
    src/nmea0183/latlong.cpp
    src/nmea0183/long.cpp
    src/nmea0183/nmea0183.cpp
    src/nmea0183/nmea0183.hpp
    src/nmea0183/Response.hpp
    src/nmea0183/response.cpp
    src/nmea0183/RMB.hpp
    src/nmea0183/rmb.cpp
    src/nmea0183/Sentence.hpp
    src/nmea0183/sentence.cpp
    src/nmea0183/talkerid.cpp
    src/nmea0183/RMC.HPP
    src/nmea0183/rmc.cpp
    src/nmea0183/hexvalue.cpp
    src/nmea0183/lat.cpp
    src/nmea0183/expid.cpp
    src/nmea0183/ebl.cpp
    src/nmea0183/ebl.hpp
    src/nmea0183/wpl.hpp
    src/nmea0183/wpl.cpp
    src/nmea0183/rte.hpp
    src/nmea0183/rte.cpp
    src/nmea0183/hdt.hpp
    src/nmea0183/hdt.cpp
    src/nmea0183/hdg.hpp
    src/nmea0183/hdg.cpp
    src/nmea0183/hdm.hpp
    src/nmea0183/hdm.cpp
    src/nmea0183/gll.hpp
    src/nmea0183/gll.cpp
    src/nmea0183/vtg.hpp
    src/nmea0183/vtg.cpp
    src/nmea0183/gga.hpp
    src/nmea0183/gga.cpp
    src/nmea0183/gsv.hpp
    src/nmea0183/gsv.cpp
    src/nmea0183/dbt.cpp
    src/nmea0183/dbt.hpp
    src/nmea0183/dpt.cpp
    src/nmea0183/dpt.hpp
    src/nmea0183/mtw.cpp
    src/nmea0183/mtw.hpp
    src/nmea0183/mwv.cpp
    src/nmea0183/mwv.hpp
    src/nmea0183/vhw.hpp
    src/nmea0183/vhw.cpp
    src/nmea0183/vwr.cpp
    src/nmea0183/vwr.hpp
    src/nmea0183/zda.cpp
    src/nmea0183/zda.hpp
    src/nmea0183/rsa.hpp
    src/nmea0183/rsa.cpp
    src/nmea0183/SatInfo.h
    src/nmea0183/mwd.cpp
    src/nmea0183/mwd.hpp
    src/nmea0183/vwt.cpp
    src/nmea0183/vwt.hpp
    src/nmea0183/mta.cpp
    src/nmea0183/mta.hpp
    src/nmea0183/vlw.cpp
    src/nmea0183/vlw.hpp
    src/nmea0183/mda.cpp
    src/nmea0183/mda.hpp
    src/nmea0183/rfa.cpp
    src/nmea0183/rfa.hpp
    src/nmea0183/rla.cpp
    src/nmea0183/rla.hpp
)

set(SRC ${SRC_ROPELESS} ${SRC_NMEA0183} ${SRC_DATA})


if(QT_ANDROID)
#  set(SRC ${SRC} src/androidSupport.cpp)
endif(QT_ANDROID)

set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DocpnUSE_GL)

set(PKG_API_LIB api-17)  #  A directory in libs/ e. g., api-17 or api-16

macro(late_init)
  # Perform initialization after the PACKAGE_NAME library, compilers
  # and ocpn::api is available.
  if (ARCH STREQUAL "armhf")
    add_definitions(-DOCPN_ARMHF)
  endif ()
  #target_include_directories(${PACKAGE_NAME} PRIVATE libs/gdal/src src)
endmacro ()

macro(add_plugin_libraries)
  add_subdirectory("opencpn-libs/opencpn-glu")
  target_link_libraries(${PACKAGE_NAME} opencpn::glu)

  #add_subdirectory("libs/cpl")
  #target_link_libraries(${PACKAGE_NAME} ocpn::cpl)

  #add_subdirectory("libs/dsa")
  #target_link_libraries(${PACKAGE_NAME} ocpn::dsa)

  #add_subdirectory("libs/wxJSON")
  #target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)

  #add_subdirectory("libs/iso8211")
  #target_link_libraries(${PACKAGE_NAME} ocpn::iso8211)

  #add_subdirectory("libs/tinyxml")
  #target_link_libraries(${PACKAGE_NAME} ocpn::tinyxml)

  #add_subdirectory("libs/zlib")
  #target_link_libraries(${PACKAGE_NAME} ocpn::zlib)

  #add_subdirectory("opencpn-libs/opencpn-glu")
  #target_link_libraries(${PACKAGE_NAME} opencpn::glu)

  #add_subdirectory("libs/wxcurl")
  #target_link_libraries(${PACKAGE_NAME} ocpn::wxcurl)

  #add_subdirectory("libs/oeserverd")

endmacro ()

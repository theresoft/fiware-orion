# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es

SET (SOURCES
    rapidjson.cpp
)

SET (HEADERS
)

#SET (STATIC_LIBS
#    convenience
#    ngsi9
#    ngsi10
#    ngsi
#    orionTypes
#    lm
#    common
#    serviceRoutines
#)

SET (STATIC_LIBS
    common
    mongoBackend
    ngsiNotify
    serviceRoutines
    mongoBackend
    rest
    jsonParse
    xmlParse
    convenience
    ngsi9
    ngsi10
    ngsi
    parse
    orionTypes
    common
    lm
    pa
    curl
)

# Include directories
# ------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/app")
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
include_directories("${RAPIDXML_INCLUDE}")

# Lib directories
# ------------------------------------------------------------
link_directories("/usr/local/lib/")
link_directories("/usr/lib64/")
link_directories("/usr/lib/x86_64-linux-gnu")

# Executable declaration
# ------------------------------------------------------------

ADD_EXECUTABLE(rapidjson ${SOURCES} ${HEADERS})

IF ((${DISTRO} STREQUAL "Ubuntu_13.04") OR (${DISTRO} STREQUAL "Ubuntu_12.04.1_LTS") OR (${DISTRO} STREQUAL "Ubuntu_12.04.2_LTS"))
  MESSAGE("rapidjson: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt -lpthread -lssl -lcrypto -lgnutls -lgcrypt)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_13.10")
  MESSAGE("rapidjson: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread -lssl -lcrypto -lgnutls -lgcrypt)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.04.1_LTS")
  # FIXME P4: Issue #476: remove or keep?
  MESSAGE("rapidjson: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread -lssl -lcrypto -lgnutls -lgcrypt)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.04.2_LTS")
  # FIXME P4: Issue #476: remove or keep?
  MESSAGE("rapidjson: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lboost_regex -lpthread -lssl -lcrypto -lgnutls -lgcrypt)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.10")
  MESSAGE("rapidjson: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread -lssl -lcrypto -lgnutls -lgcrypt)
ELSEIF(${DISTRO} STREQUAL "openSUSE_13.1")
  MESSAGE("rapidjson: openSUSE DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lpthread -lssl -lcrypto)
ELSEIF(${DISTRO} MATCHES "CentOS_6.*")
  MESSAGE("rapidjson: CentOS DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lboost_regex-mt)

ELSEIF(${DISTRO} STREQUAL "Fedora_20")
  MESSAGE("rapidjson: Fedora DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread)
ELSEIF(${DISTRO} MATCHES "Debian_.*")
  MESSAGE("rapidjson: Debian DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread -lboost_regex -lssl -lcrypto)
ELSE()
  MESSAGE("rapidjson: Other DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(rapidjson ${STATIC_LIBS} -lmicrohttpd -lmongoclient -lboost_thread -lboost_filesystem -lboost_system -lpthread)
ENDIF()

INSTALL(TARGETS               rapidjson
        RUNTIME DESTINATION   bin)

#===-- lib/abserio/CMakeLists.txt - Biblioteca de abstracción para IO Serial  -------------------------*- CMake -*-===//
#
# Copyright (c) 2018 Oever González
#
#  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
#                                 the License. You may obtain a copy of the License at
#
#                                      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software  distributed under the License is distributed on
#   an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
#                     specific language governing permissions and limitations under the License.
#
#===---------------------------------------------------------------------------------------------------------------===//
#
# Esta biblioteca ofrece un "driver" abstracto para puertos seriales en Windows (Win32) y macOS/Linux (POSIX).
#
#===---------------------------------------------------------------------------------------------------------------===//

# El nombre la biblioteca
SET ( THIS_LIB_NAME abserio )

# Los archivos dependientes de la plataforma
SET ( LIB_PLATFORM_SOURCES "" )

IF ( WIN32 )
  SET ( LIB_PLATFORM_SOURCES win_alloc.c )
ELSEIF ( UNIX )
  SET ( LIB_PLATFORM_SOURCES posix_alloc.c )
ELSE ()
  MESSAGE ( FATAL_ERROR
            "This library is supported on the following platforms: POSIX like macOS or Linux and Win32." )
ENDIF ()

# Dependemos de glib
PKG_CHECK_MODULES ( GLIB REQUIRED glib-2.0 )

# Agrega los encabezados y las bibliotecas de glib
LINK_DIRECTORIES ( ${GLIB_LIBRARY_DIRS} )
# Banderas extras para LD
STRING ( REPLACE ";" " " GLIB_LDFLAGS_LD "${GLIB_LDFLAGS_OTHER}" )
SET ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GLIB_LDFLAGS_LD}" )


# Agrega el ejecutable
ADD_LIBRARY ( ${THIS_LIB_NAME} STATIC EXCLUDE_FROM_ALL ${LIB_PLATFORM_SOURCES}
              abserio.h
              const.c )

# Agrega los encabezados y las bibliotecas de glib
TARGET_INCLUDE_DIRECTORIES ( ${THIS_LIB_NAME} PRIVATE ${GLIB_INCLUDE_DIRS} )
TARGET_LINK_LIBRARIES ( ${THIS_LIB_NAME} ${GLIB_LIBRARIES} )

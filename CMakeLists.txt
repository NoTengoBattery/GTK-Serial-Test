#===-- CMakeLists.txt - Proyecto principal  -----------------------------------------------------------*- CMake -*-===//
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
# Este módulo principal define el proyecto de CMake. Este proyecto tiene las siguientes dependencias:
#   -> CMake 3.9 o superior
#   -> pkg-config
#   -> Compilador nativo de C compatible con C11
#   -> GTK+ 3.0
#   -> Cualquier generador soportado por CMake y su respectivo ambiente
#
# Este programa funciona en Windows, macOS y Linux
#
#===---------------------------------------------------------------------------------------------------------------===//


CMAKE_MINIMUM_REQUIRED ( VERSION 3.9 )
IF ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
  MESSAGE ( FATAL_ERROR "In-source builds are not supported." )
ENDIF ()

# Exigir el estándar de C para el compilador nativo
SET ( CMAKE_C_STANDARD 11 )
SET ( CMAKE_C_STANDARD_REQUIRED ON )

# Usar pkg-config para encontrar GTK.
#   -> La mayoría de gestores de paquetes de Linux soportan pkg-config
#   -> En macOS, pkg-config funciona bien con GTK mediante macports
#      En Mac, siempre instalar la variante +quartz de GTK
#   -> En Windows, MSYS2 para Mingw64 funciona bien con pkg-config y GTK
#      GTK solamente funciona desde la terminal Mingw32/64, no desde la de MSYS
FIND_PACKAGE ( PkgConfig REQUIRED )

# Busca `gtk+-3.0` en pkg-config
PKG_CHECK_MODULES ( GTK3 REQUIRED gtk+-3.0 )

# Agrega los encabezados y las bibliotecas de GTK
INCLUDE_DIRECTORIES ( ${GTK3_INCLUDE_DIRS} )
LINK_DIRECTORIES ( ${GTK3_LIBRARY_DIRS} )
# Agrega otras banderas de GTK
ADD_DEFINITIONS ( ${GTK3_CFLAGS_OTHER} )
# Banderas extras para LD
STRING ( REPLACE ";" " " GTK3_LDFLAGS_LD "${GTK3_LDFLAGS_OTHER}" )
SET ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GTK3_LDFLAGS_LD}" )

# Proyecto de CMake para C
PROJECT ( GTK_SERIAL C )

ADD_SUBDIRECTORY ( src )


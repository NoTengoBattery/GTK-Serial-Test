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
#   -> glib2
#   -> Cualquier generador soportado por CMake y su respectivo ambiente
#
# Este programa funciona en Windows, macOS y Linux.
#
# # macOS
#   Es necesario instalar macports. Macports, a su vez, tiene como dependencia Xcode. Esto quiere decir que hay que
#   instalar Xcode y después macports.
#
#   Desde macports se puede instalar GTK (variante Quartz):
#     ->  `port install gtk3 +quartz-x11`
#   También se puede conseguir CMake:
#     ->  `port install cmake`
#   `pkg-config` y `glib2` son dependencias de GTK3, no hace falta instalarlos por separado.
#   Apple provee una versión de Clang junto con Xcode y el SDK de macOS (Cocoa/Quartz), la cual funciona tal cual viene
#   De la misma cuenta, Apple provee versiones de Git y de Make.
#
# # Windows
#   GTK no se puede desarrollar directamente sobre Windows. El primer paso fundamental es adquirir el entorno de
#   desarrollo Mingw64. Este se consigue mediante [MSYS2](https://www.msys2.org). Esta aplicación no soporta
#   (oficialmente) ningún otro entorno.
#
#   Luego de configurar MSYS2, desde la terminal de Mingw64 o Mingw32 se deben instalar los siguientes paquetes para
#   Windows de 64 bits (el entorno mínimo). El compilador y make se pueden instalar en un entorno más completo mediante
#   el metapaquete `toolchain` y `base-devel`.
#     Instalar pgk-config desde el paquete
#       -> `mingw-w64-x86_64-pkg-config`
#     Instalar CMake desde el paquete
#       -> `mingw-w64-x86_64-cmake`
#     Instalar un compilador, ya sea GCC desde el paquete
#       -> `mingw-w64-x86_64-gcc`
#     o Clang desde el paquete
#       -> `mingw-w64-x86_64-clang`
#     Es buena idea instalar un depurador (opcional) desde los paquetes, para LLDB
#       -> `mingw-w64-x86_64-lldb`
#     o GDB con el paquete
#       -> `mingw-w64-x86_64-gdb`
#     Se necesita un generador para CMake. Se puede usar ninja, pero make es más conveniente. Se instala desde el
#     paquete
#       -> `mingw-w64-x86_64-make`
#     Finalmente, se instala GTK 3 desde el paquete
#       -> `mingw-w64-x86_64-gtk3`
#     GTK tiene como dependencia a glib, así que no hace falta instalar por separado
#
# # Linux
#   Los paquetes necesarios son:
#     -> cmake
#     -> base-devel
#     -> gtk3
#     -> glib
#     -> pkg-config
#     -> clang/gcc
#
#===---------------------------------------------------------------------------------------------------------------===//


CMAKE_MINIMUM_REQUIRED ( VERSION 3.9 )

# Proyecto de CMake para C
PROJECT ( GTK_SERIAL C )

IF ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
  MESSAGE ( FATAL_ERROR "In-source builds are not supported." )
ENDIF ()

# Exigir el estándar de C para el compilador nativo
SET ( CMAKE_C_STANDARD 11 )
SET ( CMAKE_C_STANDARD_REQUIRED ON )

# Usar pkg-config para encontrar GTK.
#   -> La mayoría de gestores de paquetes de Linux soportan pkg-config
#   -> En macOS, pkg-config funciona bien con GTK mediante macports
#      En Mac, siempre instalar la variante +quartz-x11 de GTK (tiene menos dependencias y se ve mejor)
#   -> En Windows, MSYS2 para Mingw64 funciona bien con pkg-config y GTK
#      GTK solamente funciona desde la terminal Mingw32/64, no desde la de MSYS
#      Como siempre, en Windows se deben incluir las DLL de MSYS, GTK y todas sus dependencias con el .exe
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

# Contiene las bibliotecas que se compilan in-source
ADD_SUBDIRECTORY ( lib )
INCLUDE_DIRECTORIES ( lib )
# Contiene el proyecto principal
ADD_SUBDIRECTORY ( src )


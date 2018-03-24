#===-- src/CMakeLists.txt - Aplicación principal  -----------------------------------------------------*- CMake -*-===//
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
# Este sub-directorio de CMake contiene la *aplicación* principal.
#
#===---------------------------------------------------------------------------------------------------------------===//

# El nombre del ejecutable
SET ( THIS_EXE_NAME Serial )

# Agrega el ejecutable
ADD_EXECUTABLE ( ${THIS_EXE_NAME}
                 main.c )
TARGET_LINK_LIBRARIES ( ${THIS_EXE_NAME} ${GTK3_STATIC_LIBRARIES} )

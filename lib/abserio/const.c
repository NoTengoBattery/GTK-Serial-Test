//===-- lib/abserio/const.c - Constantes ------------------------------------------------------------------*- C -*-===//
//
// Copyright (c) 2018 Oever González
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
//                                 the License. You may obtain a copy of the License at
//
//                                      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software  distributed under the License is distributed on
//  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
//                    specific language governing permissions and limitations under the License.
//
//===--------------------------------------------------------------------------------------------------------------===//
///
///
///
//===--------------------------------------------------------------------------------------------------------------===//
#ifdef _WIN32
const long bauds[15] = {110,
                        300,
                        600,
                        1200,
                        2400,
                        4800,
                        9600,
                        14400,
                        19200,
                        38400,
                        57600,
                        115200,
                        128000,
                        256000,
                        0x00};
#else
const long bauds[15] = {110,
                        300,
                        600,
                        1200,
                        2400,
                        4800,
                        9600,
                        14400,
                        19200,
                        38400,
                        57600,
                        115200,
                        0x00};

#endif //_WIN32

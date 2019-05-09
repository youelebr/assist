##
#   Copyright (C) 2004 - 2016 Université de Versailles Saint-Quentin-en-Yvelines (UVSQ)
#
#   This file is part of MAQAO.
#
#  MAQAO is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public License
#   as published by the Free Software Foundation; either version 3
#   of the License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
##


FILE(GLOB_RECURSE
   files_s2s
   s2s_c.c
   main.cpp
   s2sLoop.cpp
   s2sFunction.cpp
   s2sDefines.cpp
   api.cpp
)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../Rose/include")
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../boostRose/include")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}) 

SET(C_STATIC_FLAGS "${C_STATIC_FLAGS} -w -I${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/include -L${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/lib")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -I${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/include -L${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/lib")
SET(CXX_STATIC_FLAGS "${CXX_STATIC_FLAGS} -w -I${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/include -L${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/lib")

SET(C_DYNAMIC_FLAGS "${C_DYNAMIC_FLAGS} -w -I${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/include -L${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/lib")
SET(CXX_DYNAMIC_FLAGS "${CXX_DYNAMIC_FLAGS} -w -I${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/include -L${CMAKE_SOURCE_DIR}/src/plugins/s2s/lib/Rose/lib")

# Create the s2s_c_lua object for STATIC libraries/binaries #
ADD_LIBRARY(s2s_c_lua-obj-static 				OBJECT  ${files_s2s})
SET_TARGET_PROPERTIES(s2s_c_lua-obj-static  	PROPERTIES COMPILE_FLAGS "${C_STATIC_FLAGS} ${CXX_STATIC_FLAGS}")
ADD_DEPENDENCIES (s2s_c_lua-obj-static 		${DO_LUA_DEPENDENCY})
#TARGET_LINK_LIBRARIES(s2s_c_lua-lib-static       maqao-lib-static rose )


# Create the s2s_c_lua object for DYNAMIC libraries/binaries #
ADD_LIBRARY(s2s_c_lua-obj-dyn                 OBJECT ${files_s2s})
SET_TARGET_PROPERTIES(s2s_c_lua-obj-dyn       PROPERTIES COMPILE_FLAGS "${C_DYNAMIC_FLAGS} ${CXX_DYNAMIC_FLAGS}")
ADD_DEPENDENCIES(s2s_c_lua-obj-dyn            ${DO_LUA_DEPENDENCY})

### --- Create the dynamic s2s_c_lua library --- ###
ADD_LIBRARY(s2s_c_lua-lib-dyn                 SHARED $<TARGET_OBJECTS:s2s_c_lua-obj-dyn>)
SET_TARGET_PROPERTIES(s2s_c_lua-lib-dyn       PROPERTIES COMPILE_FLAGS "${C_DYNAMIC_FLAGS} ${CXX_DYNAMIC_FLAGS}")
SET_TARGET_PROPERTIES(s2s_c_lua-lib-dyn       PROPERTIES PREFIX "" OUTPUT_NAME s2s_c)

# Link the dynamic s2s_c_lua library to the required libraries #
TARGET_LINK_LIBRARIES(s2s_c_lua-lib-dyn       maqao-lib-dyn rose)


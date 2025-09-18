include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc
)

file (GLOB SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	*.c
	*.h
)

if (WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

if (BUILD_DLL)
	add_library (libmath SHARED ${SRC})
	if (WIN32)
		target_link_libraries (libmath libmsl)
	else (WIN32)
		target_link_libraries (libmath libmsl m)
	endif (WIN32)
else (BUILD_DLL)
	add_library (libmath STATIC ${SRC})
endif (BUILD_DLL)

set_target_properties(libmath
	PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
)

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
	add_library (libstring SHARED ${SRC})
	target_link_libraries (libstring libmsl)
else (BUILD_DLL)
	add_library (libstring STATIC ${SRC})
endif (BUILD_DLL)

set_target_properties(libstring
	PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
)

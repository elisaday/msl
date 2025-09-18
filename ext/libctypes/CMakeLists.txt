include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../../inc)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/libffi_msvc)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/libffi/src/x86)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/libffi/include)

file (GLOB SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	*.c
	*.h
)

if (WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

add_definitions(-DFFI_BUILDING)

if (WIN32 AND BUILD_DLL)
		if (MSVC11)
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libffi_msvc/libffi_msvc2012)
	elseif (MSVC10)
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libffi_msvc/libffi_msvc2010)
	elseif (MSVC90)
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libffi_msvc/libffi_msvc2008)
	elseif (MSVC71)
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libffi_msvc/libffi_msvc2003)
	endif ()

	add_library (libctypes SHARED ${SRC})
	target_link_libraries (libctypes libmsl)

	if (MSVC11)
		set_target_properties (libctypes
			PROPERTIES
			LINK_FLAGS "${LINK_FLAGS} /SAFESEH:NO"
		)
	endif ()

	target_link_libraries (libctypes libffi_msvc.lib)
else ()
	add_library (libctypes STATIC ${SRC})
endif ()

set_target_properties(libctypes
	PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
)

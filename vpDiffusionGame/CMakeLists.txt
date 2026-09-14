#-----------------------------------------------------------------------------
#	vpDiffusionGame
#
#	Project Script
#-----------------------------------------------------------------------------

cmake_minimum_required( VERSION 3.20 FATAL_ERROR )

include( "${SRCDIR}/sdk/JackPluginSDK.cmake" )

set( DIFFUSIONGAME_SOURCE_FILES )
BEGIN_SRC( DIFFUSIONGAME_SOURCE_FILES "Source Files" )
	SRC_GRP(
		SOURCES
		#{
			"main.cpp"

			"../common/DataSerializer.cpp"
			"../common/vstdlib_static.cpp"
		#}
	)
END_SRC( DIFFUSIONGAME_SOURCE_FILES "Source Files" )

set( DIFFUSIONGAME_HEADER_FILES )
BEGIN_SRC( DIFFUSIONGAME_HEADER_FILES "Header Files" )
	SRC_GRP(
		SOURCES
		#{
			"../common/DataSerializer.h"
			"../common/vstdlib_static.h"
		#}
	)
END_SRC( DIFFUSIONGAME_HEADER_FILES "Header Files" )

add_library(
	diffusiongame MODULE
	${DIFFUSIONGAME_SOURCE_FILES} ${DIFFUSIONGAME_HEADER_FILES}
)

install_library(
	TARGET diffusiongame
	INSTALL_OUTNAME "vpDiffusionGame"
	INSTALL_DEST "${CMAKE_INSTALL_PREFIX}"
)

target_include_directories(
	diffusiongame PRIVATE

	"."
	"../common"
)

target_use_jackpluginsdk( diffusiongame "${SRCDIR}/sdk" )

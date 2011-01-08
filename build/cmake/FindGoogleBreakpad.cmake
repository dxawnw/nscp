# -*- cmake -*-

# - Find Google BreakPad
# Find the Google BreakPad includes and library
# This module defines
#  BREAKPAD_EXCEPTION_HANDLER_INCLUDE_DIR, where to find exception_handler.h, etc.
#  BREAKPAD_EXCEPTION_HANDLER_LIBRARIES, the libraries needed to use Google BreakPad.
#  BREAKPAD_EXCEPTION_HANDLER_FOUND, If false, do not try to use Google BreakPad.
# also defined, but not for general use are
#  BREAKPAD_EXCEPTION_HANDLER_LIBRARY, where to find the Google BreakPad library.

FIND_PATH(BREAKPAD_INCLUDE_DIR google_breakpad/client/breakpad_types.h)


#google-breakpad-common.lib
#google-breakpad-crash_generation_client.lib
#google-breakpad-crash_generation_server.lib
#google-breakpad-crash_report_sender.lib
#google-breakpad-exception_handler.lib
#google-breakpad-gmock.lib
#google-breakpad-gtest.lib

IF(NOT GoogleBreakpad_FIND_COMPONENTS)
	SET(GoogleBreakpad_FIND_COMPONENTS common exception_handler)
ENDIF(NOT GoogleBreakpad_FIND_COMPONENTS)
IF(NOT BREAKPAD_LIBRARY_PREFIX)
	SET(BREAKPAD_LIBRARY_PREFIX "")
ENDIF(NOT BREAKPAD_LIBRARY_PREFIX)
IF(NOT BREAKPAD_LIBRARY_SUFFIX)
	SET(BREAKPAD_LIBRARY_SUFFIX "")
ENDIF(NOT BREAKPAD_LIBRARY_SUFFIX)
IF(NOT BREAKPAD_LIBRARY_PREFIX_DEBUG)
	SET(BREAKPAD_LIBRARY_PREFIX_DEBUG ${BREAKPAD_LIBRARY_PREFIX})
ENDIF(NOT BREAKPAD_LIBRARY_PREFIX_DEBUG)
IF(NOT BREAKPAD_LIBRARY_SUFFIX_DEBUG)
	SET(BREAKPAD_LIBRARY_SUFFIX_DEBUG ${BREAKPAD_LIBRARY_SUFFIX})
ENDIF(NOT BREAKPAD_LIBRARY_SUFFIX_DEBUG)

MESSAGE(STATUS "Breakpad config: ${BREAKPAD_LIBRARY_PREFIX}...${BREAKPAD_LIBRARY_SUFFIX}, ${BREAKPAD_LIBRARY_PREFIX_DEBUG}...${BREAKPAD_LIBRARY_SUFFIX_DEBUG}" )
SET(BREAKPAD_FOUND TRUE)
FOREACH(COMPONENT ${GoogleBreakpad_FIND_COMPONENTS})
    string(TOUPPER ${COMPONENT} UPPERCOMPONENT)
	FIND_LIBRARY(BREAKPAD_${UPPERCOMPONENT}_LIBRARY_RELEASE NAMES ${BREAKPAD_LIBRARY_PREFIX}${COMPONENT}${BREAKPAD_LIBRARY_SUFFIX})
	FIND_LIBRARY(BREAKPAD_${UPPERCOMPONENT}_LIBRARY_DEBUG NAMES ${BREAKPAD_LIBRARY_PREFIX_DEBUG}${COMPONENT}${BREAKPAD_LIBRARY_SUFFIX_DEBUG})
	IF(BREAKPAD_${UPPERCOMPONENT}_LIBRARY_RELEASE AND BREAKPAD_${UPPERCOMPONENT}_LIBRARY_DEBUG)
		SET(BREAKPAD_${UPPERCOMPONENT}_FOUND TRUE)
		SET(BREAKPAD_${UPPERCOMPONENT}_LIBRARY optimized ${BREAKPAD_${UPPERCOMPONENT}_LIBRARY_RELEASE} debug ${BREAKPAD_${UPPERCOMPONENT}_LIBRARY_DEBUG})
		set(BREAKPAD_${UPPERCOMPONENT}_LIBRARY ${BREAKPAD_${UPPERCOMPONENT}_LIBRARY} CACHE FILEPATH "The breakpad ${UPPERCOMPONENT} library")
	ELSE(BREAKPAD_${UPPERCOMPONENT}_LIBRARY_RELEASE AND BREAKPAD_${UPPERCOMPONENT}_LIBRARY_DEBUG)
		SET(BREAKPAD_FOUND FALSE)
		SET(BREAKPAD_${UPPERCOMPONENT}_FOUND FALSE)
		SET(BREAKPAD_${UPPERCOMPONENT}_LIBRARY "${BREAKPAD_${UPPERCOMPONENT}_LIBRARY_RELEASE-NOTFOUND}")
	ENDIF(BREAKPAD_${UPPERCOMPONENT}_LIBRARY_RELEASE AND BREAKPAD_${UPPERCOMPONENT}_LIBRARY_DEBUG)
ENDFOREACH(COMPONENT)
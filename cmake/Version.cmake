# ──────────────────────────────────────────────────────────────────────────
# Version.cmake — the version comes from the git tag, nowhere else.
#
# Included before project(). Sets:
#   STH_VERSION_NUMBER   X.Y.Z, for project() and so for the hosts (VST3,
#                        AU, CLAP and the standalone need a plain number)
#   STH_VERSION_DISPLAY  what the window shows: "0.1.0" on the tag v0.1.0,
#                        "0.1.0-3-gabc1234" three commits after it,
#                        "0.0.0-dev" before any tag (or without git)
#
# The tag is the latest vX.Y.Z reachable from HEAD (git describe), the same
# pattern that triggers release.yml. -DSTH_VERSION=X.Y.Z overrides it (both
# values then). CMake reconfigures by itself on a commit, a checkout or a new
# tag, so a local build never shows a stale version.
# ──────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)

set(STH_VERSION "" CACHE STRING "Override the version (X.Y.Z); empty: from the git tag")

if(STH_VERSION)
    if(NOT STH_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR "STH_VERSION must be X.Y.Z, got '${STH_VERSION}'")
    endif()
    set(_sth_version "${STH_VERSION}")
    set(_sth_display "${STH_VERSION}")
else()
    set(_sth_version "0.0.0")
    set(_sth_display "0.0.0-dev")
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v[0-9]*.[0-9]*.[0-9]*"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE _sth_describe
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE _sth_git_result)
        # v0.1.0, or v0.1.0-3-gabc1234 after it.
        if(_sth_git_result EQUAL 0 AND _sth_describe MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)(-.*)?$")
            set(_sth_version "${CMAKE_MATCH_1}")
            string(SUBSTRING "${_sth_describe}" 1 -1 _sth_display)
        endif()

        # Reconfigure when HEAD moves (a commit, a checkout) or a tag appears.
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --absolute-git-dir
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE _sth_git_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE _sth_git_dir_result)
        if(_sth_git_dir_result EQUAL 0)
            set(_sth_watch "${_sth_git_dir}/HEAD" "${_sth_git_dir}/packed-refs" "${_sth_git_dir}/refs/tags")
            file(READ "${_sth_git_dir}/HEAD" _sth_head)
            if(_sth_head MATCHES "^ref: ([^\n]+)")
                list(APPEND _sth_watch "${_sth_git_dir}/${CMAKE_MATCH_1}")
            endif()
            foreach(_f IN LISTS _sth_watch)
                if(EXISTS "${_f}")
                    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_f}")
                endif()
            endforeach()
        endif()
    endif()
endif()

set(STH_VERSION_NUMBER "${_sth_version}")
set(STH_VERSION_DISPLAY "${_sth_display}")
message(STATUS "StairwellToHeaven version ${STH_VERSION_DISPLAY} (hosts see ${STH_VERSION_NUMBER})")

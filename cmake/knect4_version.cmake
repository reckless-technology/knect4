# Knect 4's version, MAJOR.MINOR.BUILD, from git alone - the rule every Reckless project follows: MAJOR.MINOR is the
# nearest tag of the form vMAJOR.MINOR reachable from HEAD, made by hand (a release's own vMAJOR.MINOR.BUILD tags are
# three-component and excluded), and BUILD is the number of commits since it, the tagged commit itself being 0. Nothing
# in the tree is edited to release. Included before project(), which takes KNECT4_VERSION as the project's version, so
# PROJECT_VERSION and its parts are this number everywhere after.
#
# A checkout with no such tag reachable (a shallow clone, a fork without the tags, or the repository before its first
# vMAJOR.MINOR tag) builds as 0.0.0 with a warning. The configure runs again when HEAD moves; after `git tag`,
# reconfigure by hand.
include_guard(GLOBAL)

find_program(KNECT4_GIT_EXECUTABLE NAMES git DOC "The git that reads the version from the tags")
set(_knect4_describe "")
set(_knect4_result 1)
if(KNECT4_GIT_EXECUTABLE)
    execute_process(
        COMMAND "${KNECT4_GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" describe --tags --long
                --match "v[0-9]*.[0-9]*" --exclude "v*.*.*"
        OUTPUT_VARIABLE _knect4_describe
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _knect4_result
        ERROR_QUIET)
endif()
if(_knect4_result EQUAL 0 AND _knect4_describe MATCHES "^v([0-9]+)\\.([0-9]+)-([0-9]+)-g[0-9a-f]+$")
    set(KNECT4_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
else()
    message(WARNING "No vMAJOR.MINOR tag is reachable from HEAD (git describe said '${_knect4_describe}'): the build "
                    "reports 0.0.0. Fetch the tags (git fetch --tags; CI checks out with fetch-depth 0).")
    set(KNECT4_VERSION "0.0.0")
endif()
message(STATUS "Knect 4 ${KNECT4_VERSION}")

# A new commit or a checkout configures again, so the number follows HEAD.
if(KNECT4_GIT_EXECUTABLE)
    execute_process(
        COMMAND "${KNECT4_GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --absolute-git-dir
        OUTPUT_VARIABLE _knect4_git_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(
        COMMAND "${KNECT4_GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --path-format=absolute
                --git-common-dir
        OUTPUT_VARIABLE _knect4_git_common_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    foreach(_knect4_ref "${_knect4_git_dir}/HEAD" "${_knect4_git_dir}/logs/HEAD" "${_knect4_git_common_dir}/packed-refs")
        if(EXISTS "${_knect4_ref}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_knect4_ref}")
        endif()
    endforeach()
endif()

# cmake/fetch_external.cmake
#
# Configure-time fetch + SHA-verify + extract for vendored upstreams that are
# not committed to the repository. Each external/<lib>/PINNED_VERSION carries
# the upstream URL, expected SHA-256, and release date; this script consumes
# those and populates external/<lib>/{src,include,...} as needed. See ADR-008.
#
# Idempotency: a marker file (external/<lib>/_extracted_marker) holds the
# expected SHA-256 of the tarball that produced the current extraction. If
# the marker is present and matches PINNED_VERSION, this script is a no-op.
#
# If the network is unreachable or the SHA does not match, this script emits
# a clear FATAL_ERROR naming the missing tarball and the manual recovery
# command, per the Phase-0 plan's "no silent fallback" rule.

set(APSIS_EXTERNAL_DIR   "${CMAKE_SOURCE_DIR}/external")
set(APSIS_EXTERNAL_CACHE "${APSIS_EXTERNAL_DIR}/_cache")
file(MAKE_DIRECTORY "${APSIS_EXTERNAL_CACHE}")

# ---------------------------------------------------------------------------
# Read a PINNED_VERSION key=value file into the calling scope.
# ---------------------------------------------------------------------------
function(_apsis_read_pinned_version path out_url out_sha out_release)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "PINNED_VERSION not found at ${path}")
  endif()
  file(STRINGS "${path}" _lines)
  foreach(line IN LISTS _lines)
    if(line MATCHES "^URL=(.*)$")
      set(_url "${CMAKE_MATCH_1}")
    elseif(line MATCHES "^SHA256=(.*)$")
      set(_sha "${CMAKE_MATCH_1}")
    elseif(line MATCHES "^RELEASE=(.*)$")
      set(_rel "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  if(NOT _url OR NOT _sha)
    message(FATAL_ERROR "PINNED_VERSION at ${path} missing URL= or SHA256=")
  endif()
  set(${out_url}     "${_url}" PARENT_SCOPE)
  set(${out_sha}     "${_sha}" PARENT_SCOPE)
  set(${out_release} "${_rel}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# Download URL to dest; verify SHA-256 matches expected. Skips if the file
# already exists with the right SHA. Errors out otherwise (no silent fallback).
# ---------------------------------------------------------------------------
function(_apsis_download_and_verify url dest expected_sha)
  if(EXISTS "${dest}")
    file(SHA256 "${dest}" _have_sha)
    if(_have_sha STREQUAL expected_sha)
      return()
    endif()
    message(STATUS
      "[apsis] cached tarball ${dest} SHA mismatch "
      "(have ${_have_sha}, expected ${expected_sha}); re-downloading.")
    file(REMOVE "${dest}")
  endif()

  message(STATUS "[apsis] downloading ${url}")
  file(DOWNLOAD "${url}" "${dest}"
       SHOW_PROGRESS
       STATUS _dl_status
       TIMEOUT 300
       TLS_VERIFY ON)
  list(GET _dl_status 0 _dl_code)
  list(GET _dl_status 1 _dl_msg)
  if(NOT _dl_code EQUAL 0)
    file(REMOVE "${dest}")
    message(FATAL_ERROR
      "[apsis] failed to download ${url}: ${_dl_msg}\n"
      "Manual recovery: curl -L '${url}' -o '${dest}'")
  endif()

  file(SHA256 "${dest}" _have_sha)
  if(NOT _have_sha STREQUAL expected_sha)
    file(REMOVE "${dest}")
    message(FATAL_ERROR
      "[apsis] SHA-256 mismatch for ${dest}\n"
      "  expected: ${expected_sha}\n"
      "  actual:   ${_have_sha}\n"
      "Either upstream re-issued the artefact or the URL is wrong. "
      "Refusing to proceed.")
  endif()
endfunction()

# ---------------------------------------------------------------------------
# Extract a tarball into a temp dir, then move a single sub-path into the
# desired layout under external/<lib>/.
# Args:
#   tarball       absolute path to .tar.gz / .tar.Z (handled transparently)
#   work_dir      scratch dir; recreated on each call
#   strip_paths   list of "src_subpath -> dest_subpath" entries.
# ---------------------------------------------------------------------------
function(_apsis_extract_tar tarball work_dir)
  if(EXISTS "${work_dir}")
    file(REMOVE_RECURSE "${work_dir}")
  endif()
  file(MAKE_DIRECTORY "${work_dir}")

  get_filename_component(_ext "${tarball}" EXT)
  if(_ext STREQUAL ".Z")
    # tar.Z — CMake's file(ARCHIVE_EXTRACT) does not handle .Z compression.
    # Use uncompress (POSIX) or gunzip (which also handles .Z).
    find_program(_uncompress_prog NAMES uncompress gunzip
                 DOC "uncompress / gunzip for .Z tarballs")
    if(NOT _uncompress_prog)
      message(FATAL_ERROR
        "[apsis] cannot extract ${tarball}: need 'uncompress' or 'gunzip' "
        "on PATH to handle .tar.Z. Install it (e.g. 'apt install ncompress').")
    endif()
    # Copy first so we don't consume the cached tarball; uncompress removes the .Z.
    set(_staged "${work_dir}/_staged.tar.Z")
    configure_file("${tarball}" "${_staged}" COPYONLY)
    execute_process(
      COMMAND ${_uncompress_prog} "${_staged}"
      WORKING_DIRECTORY "${work_dir}"
      RESULT_VARIABLE _rc
      OUTPUT_VARIABLE _out
      ERROR_VARIABLE  _err)
    if(NOT _rc EQUAL 0)
      message(FATAL_ERROR "[apsis] uncompress failed (rc=${_rc}): ${_err}\n${_out}")
    endif()
    set(_plain_tar "${work_dir}/_staged.tar")
  else()
    # tar.gz — let CMake handle it.
    set(_plain_tar "${tarball}")
  endif()

  file(ARCHIVE_EXTRACT INPUT "${_plain_tar}" DESTINATION "${work_dir}")
endfunction()

# ---------------------------------------------------------------------------
# Marker management — caches the SHA so a subsequent configure with no change
# is fast.
# ---------------------------------------------------------------------------
function(_apsis_marker_path libdir out)
  set(${out} "${libdir}/_extracted_marker" PARENT_SCOPE)
endfunction()

function(_apsis_marker_matches libdir expected_sha out)
  _apsis_marker_path("${libdir}" _path)
  if(NOT EXISTS "${_path}")
    set(${out} FALSE PARENT_SCOPE)
    return()
  endif()
  file(READ "${_path}" _have)
  string(STRIP "${_have}" _have)
  if(_have STREQUAL expected_sha)
    set(${out} TRUE PARENT_SCOPE)
  else()
    set(${out} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(_apsis_marker_write libdir sha)
  _apsis_marker_path("${libdir}" _path)
  file(WRITE "${_path}" "${sha}\n")
endfunction()

# ---------------------------------------------------------------------------
# Per-library populate functions. Each one:
#   1. Reads PINNED_VERSION
#   2. Skips if marker matches expected SHA AND src/ is non-empty
#   3. Downloads + SHA-verifies tarball into _cache/
#   4. Extracts to a scratch dir
#   5. Moves the upstream-specific subtree into external/<lib>/{src,include}/
#   6. Writes the marker
# ---------------------------------------------------------------------------

function(_apsis_populate_sofa)
  set(_libdir   "${APSIS_EXTERNAL_DIR}/sofa")
  set(_pinfile  "${_libdir}/PINNED_VERSION")
  _apsis_read_pinned_version("${_pinfile}" _url _sha _rel)

  set(_tarball "${APSIS_EXTERNAL_CACHE}/sofa_c-20231011.tar.gz")

  _apsis_marker_matches("${_libdir}" "${_sha}" _ok)
  if(_ok AND EXISTS "${_libdir}/src" AND EXISTS "${_libdir}/include/sofa.h")
    return()
  endif()

  _apsis_download_and_verify("${_url}" "${_tarball}" "${_sha}")

  set(_work "${APSIS_EXTERNAL_CACHE}/_extract_sofa")
  _apsis_extract_tar("${_tarball}" "${_work}")

  # Upstream layout: sofa/20231011/c/src/{*.c,sofa.h,sofam.h,t_sofa_c.c,makefile}
  set(_upstream_src "${_work}/sofa/20231011/c/src")
  if(NOT EXISTS "${_upstream_src}/sofa.h")
    message(FATAL_ERROR "[apsis] SOFA tarball layout unexpected; missing ${_upstream_src}/sofa.h")
  endif()

  # Wipe any stale extraction.
  file(REMOVE_RECURSE "${_libdir}/src" "${_libdir}/include")
  file(MAKE_DIRECTORY "${_libdir}/src" "${_libdir}/include")

  # Copy *.c into src/, drop t_sofa_c.c (the upstream test driver) so the
  # static library is library-only. Copy the two headers into include/.
  file(GLOB _all_c CONFIGURE_DEPENDS "${_upstream_src}/*.c")
  foreach(_f IN LISTS _all_c)
    get_filename_component(_name "${_f}" NAME)
    if(_name STREQUAL "t_sofa_c.c")
      continue()
    endif()
    configure_file("${_f}" "${_libdir}/src/${_name}" COPYONLY)
  endforeach()
  configure_file("${_upstream_src}/sofa.h"  "${_libdir}/include/sofa.h"  COPYONLY)
  configure_file("${_upstream_src}/sofam.h" "${_libdir}/include/sofam.h" COPYONLY)

  file(REMOVE_RECURSE "${_work}")
  _apsis_marker_write("${_libdir}" "${_sha}")
  message(STATUS "[apsis] SOFA ${_rel} extracted into ${_libdir}/")
endfunction()

function(_apsis_populate_cspice)
  set(_libdir   "${APSIS_EXTERNAL_DIR}/cspice")
  set(_pinfile  "${_libdir}/PINNED_VERSION")
  _apsis_read_pinned_version("${_pinfile}" _url _sha _rel)

  set(_tarball "${APSIS_EXTERNAL_CACHE}/cspice.tar.Z")

  _apsis_marker_matches("${_libdir}" "${_sha}" _ok)
  if(_ok AND EXISTS "${_libdir}/src" AND EXISTS "${_libdir}/include/SpiceUsr.h")
    return()
  endif()

  _apsis_download_and_verify("${_url}" "${_tarball}" "${_sha}")

  set(_work "${APSIS_EXTERNAL_CACHE}/_extract_cspice")
  _apsis_extract_tar("${_tarball}" "${_work}")

  # Upstream layout: cspice/{src/cspice/*.c,*.h, src/csupport/*.c, include/Spice*.h, ...}
  set(_upstream_root "${_work}/cspice")
  if(NOT EXISTS "${_upstream_root}/include/SpiceUsr.h")
    message(FATAL_ERROR "[apsis] CSPICE tarball layout unexpected; missing ${_upstream_root}/include/SpiceUsr.h")
  endif()

  file(REMOVE_RECURSE "${_libdir}/src" "${_libdir}/include")
  file(MAKE_DIRECTORY "${_libdir}/src" "${_libdir}/include")

  # Core C library lives at src/cspice/ (the f2c-translated SPICELIB).
  # Phase 0 ships only this; csupport / utility programs are not needed for
  # apsis::cspice consumption. Phase 1 may pull csupport in if needed.
  file(GLOB _core_files CONFIGURE_DEPENDS
    "${_upstream_root}/src/cspice/*.c"
    "${_upstream_root}/src/cspice/*.h")
  foreach(_f IN LISTS _core_files)
    get_filename_component(_name "${_f}" NAME)
    configure_file("${_f}" "${_libdir}/src/${_name}" COPYONLY)
  endforeach()

  # Public headers used by consumers (Spice*.h).
  file(GLOB _pub_headers CONFIGURE_DEPENDS "${_upstream_root}/include/*.h")
  foreach(_h IN LISTS _pub_headers)
    get_filename_component(_name "${_h}" NAME)
    configure_file("${_h}" "${_libdir}/include/${_name}" COPYONLY)
  endforeach()

  file(REMOVE_RECURSE "${_work}")
  _apsis_marker_write("${_libdir}" "${_sha}")
  message(STATUS "[apsis] CSPICE ${_rel} extracted into ${_libdir}/")
endfunction()

_apsis_populate_sofa()
_apsis_populate_cspice()

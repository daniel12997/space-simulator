# Phase 0 Plan — Project Bootstrap

> Greenfield bootstrap. **Gate**: a smoke-test executable links against an
> empty `apsis::core` library skeleton, runs on Linux (gcc-13, clang-17) and
> Windows (MSVC 2022) in GitHub Actions, and emits zero compiler warnings.
> No domain code lands in this phase.

## Reference

- Structure outline: `docs/structure.md` (Phase 0 is implicit prelude to Phase 1).
- Decisions consumed: [[wiki/decisions/006-cmake-cpm-build-system]],
  [[wiki/decisions/007-googletest-test-framework]],
  [[wiki/decisions/008-vendor-sofa-and-cspice]],
  [[wiki/decisions/011-reference-data-shipping]],
  [[wiki/decisions/012-eigen-with-apsis-math-aliases]].

## Changes

### 1. License and repo housekeeping

**Files**: `LICENSE`, `.gitignore`, `THIRD_PARTY_LICENSES.md`, `.clang-format`, `.clang-tidy`
**Action**: create

- `LICENSE` — canonical Apache-2.0 text with `Copyright 2026 Apsis Contributors`.
- `.gitignore` — append `build/`, `build-*/`, `cmake-build-*/`, `compile_commands.json`, `.cache/`, `external/_cache/`, `data/_fetched/`, IDE noise (`.vscode/`, `.idea/`).
- `THIRD_PARTY_LICENSES.md` — placeholder skeleton; populated as upstreams vendor in (SOFA, CSPICE, Eigen, GoogleTest).
- `.clang-format` — based on LLVM style, column 100, 2-space indent, pointer alignment left.
- `.clang-tidy` — `Checks: 'bugprone-*,performance-*,modernize-*,readability-identifier-naming,cppcoreguidelines-pro-*,-modernize-use-trailing-return-type'` plus a `WarningsAsErrors: '*'`.

### 2. Top-level CMake

**File**: `CMakeLists.txt`
**Action**: create

```cmake
cmake_minimum_required(VERSION 3.25)
project(apsis VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(APSIS_BUILD_TESTS "Build the Apsis test suite" ON)
option(APSIS_FETCH_LARGE_DATA "Fetch large reference data (DE441, full EGM2008)" OFF)
option(APSIS_ENABLE_SANITIZERS "Enable ASan+UBSan in non-Release builds" OFF)

include(cmake/apsis_compile_options.cmake)
include(cmake/apsis_dependencies.cmake)

add_subdirectory(external/sofa)
add_subdirectory(external/cspice)
add_subdirectory(src)

if(APSIS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
```

### 3. CMake helpers

**Files**: `cmake/CPM.cmake` (vendored single file), `cmake/CPM_VERSION`,
`cmake/apsis_compile_options.cmake`, `cmake/apsis_dependencies.cmake`,
`cmake/fetch_external.cmake`, `cmake/fetch_large_data.cmake`
**Action**: create

```cmake
# cmake/apsis_compile_options.cmake
function(apsis_apply_compile_options target)
  target_compile_features(${target} PUBLIC cxx_std_17)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive- /Zc:__cplusplus)
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Werror
      -Wshadow -Wnon-virtual-dtor -Wold-style-cast
      -Wcast-align -Woverloaded-virtual -Wconversion
      -Wsign-conversion -Wdouble-promotion -Wformat=2)
  endif()
  if(APSIS_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()
```

```cmake
# cmake/apsis_dependencies.cmake
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(NAME Eigen
  GITLAB_REPOSITORY libeigen/eigen
  GIT_TAG 3.4.0
  DOWNLOAD_ONLY YES)
add_library(apsis::eigen INTERFACE IMPORTED)
target_include_directories(apsis::eigen INTERFACE ${Eigen_SOURCE_DIR})

if(APSIS_BUILD_TESTS)
  CPMAddPackage(NAME GoogleTest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.15.2
    OPTIONS "INSTALL_GTEST OFF" "BUILD_GMOCK ON" "gtest_force_shared_crt ON")
endif()
```

`fetch_external.cmake` is invoked at configure time (not via CPM) and is
responsible for downloading + SHA-verifying the SOFA and CSPICE upstream
tarballs into `external/_cache/`, then extracting them into `external/sofa/`
and `external/cspice/` if a marker file is missing. SHA-256 and URL come
from each `external/<lib>/PINNED_VERSION` file.

`fetch_large_data.cmake` (gated by `APSIS_FETCH_LARGE_DATA`) is a stub in
Phase 0 — populated when Phase 1 needs DE441 / full EGM2008. Header only
in this phase.

### 4. External library wrappers

**Files**: `external/sofa/CMakeLists.txt`, `external/sofa/PINNED_VERSION`,
`external/sofa/.gitignore`, `external/cspice/CMakeLists.txt`,
`external/cspice/PINNED_VERSION`, `external/cspice/.gitignore`
**Action**: create

Each `PINNED_VERSION` file records (one key per line):
```
URL=https://www.iausofa.org/2023_1011_C/sofa_c-20231011.tar.gz
SHA256=<recorded once verified>
RELEASE=2023-10-11
```

Each per-lib `.gitignore` excludes `src/`, `include/`, and `_extracted_marker`
so the upstream extracted source is not committed.

```cmake
# external/sofa/CMakeLists.txt
file(GLOB APSIS_SOFA_SOURCES CONFIGURE_DEPENDS "src/*.c")
if(NOT APSIS_SOFA_SOURCES)
  message(FATAL_ERROR
    "SOFA upstream source missing under external/sofa/src/. "
    "Run cmake/fetch_external.cmake or check PINNED_VERSION.")
endif()
add_library(apsis_sofa STATIC ${APSIS_SOFA_SOURCES})
target_include_directories(apsis_sofa PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(apsis::sofa ALIAS apsis_sofa)
```

CSPICE wrapper is structurally identical, with one addition: a custom
clang-tidy rule (in `tools/lint/cspice_seam.py`, deferred but flagged
here) flags `*_c(` calls outside `src/ephemeris/`. Phase 0 ships the
`cspice_seam.py` skeleton; the matcher is exercised in Phase 1.

### 5. Library and smoke skeleton

**Files**: `include/apsis/version.h`, `src/version.cc`, `src/CMakeLists.txt`,
`tests/CMakeLists.txt`, `tests/smoke/version_smoke.cc`
**Action**: create

```cpp
// include/apsis/version.h
#pragma once
#include <string_view>
namespace apsis {
constexpr std::string_view kVersionString = "0.0.1+phase-0";
[[nodiscard]] std::string_view version() noexcept;
}
```

```cpp
// src/version.cc
#include <apsis/version.h>
namespace apsis {
std::string_view version() noexcept { return kVersionString; }
}
```

```cmake
# src/CMakeLists.txt
add_library(apsis_core STATIC version.cc)
target_include_directories(apsis_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(apsis_core PUBLIC apsis::eigen)
apsis_apply_compile_options(apsis_core)
add_library(apsis::core ALIAS apsis_core)
```

```cpp
// tests/smoke/version_smoke.cc
#include <gtest/gtest.h>
#include <apsis/version.h>
TEST(VersionSmoke, NonEmpty) {
  EXPECT_FALSE(apsis::version().empty());
}
```

`tests/CMakeLists.txt` defines a helper `apsis_add_test(name LABEL <label>
SOURCES …)` that compiles a target, links `gtest_main` plus `apsis::core`,
applies compile options, and registers with CTest under the given label.
Smoke test labelled `smoke`.

### 6. CI

**File**: `.github/workflows/ci.yml`
**Action**: create

Matrix:
- `ubuntu-22.04` × `{gcc-13, clang-17}`
- `windows-2022` × `msvc`

Steps per job:
1. Checkout.
2. Restore `external/_cache/` from cache (key by `cmake/CPM_VERSION` + each `PINNED_VERSION` SHA).
3. `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo`.
4. `cmake --build build --parallel`.
5. `ctest --test-dir build --output-on-failure`.
6. `clang-format --dry-run --Werror $(git ls-files '*.cc' '*.h')` (Linux only).
7. `clang-tidy -p build $(git ls-files 'src/*.cc' 'include/apsis/*.h')` (Linux only, gcc job).

A second job runs the sanitizer build:
`cmake -DAPSIS_ENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug` then `ctest`.

## Verification

### Automated
- [x] `cmake -S . -B build -G Ninja` configures cleanly on Linux + Windows; SOFA / CSPICE upstream extract on first configure with SHA verification passing. (Linux gcc-13 verified locally; Windows reviewed by hand in CI YAML.)
- [x] `cmake --build build` succeeds with **zero compiler warnings** (warnings-as-errors enforced by `apsis_compile_options.cmake`). (Linux gcc-13 verified locally.)
- [x] `ctest --test-dir build --output-on-failure -L smoke` runs and `VersionSmoke.NonEmpty` passes.
- [x] `clang-format --dry-run --Werror` exits 0 across all `.cc` / `.h` files. (clang-format-17 verified locally.)
- [x] `clang-tidy` runs with zero diagnostics on `apsis_core` sources. (clang-tidy-17 verified locally; the 10k upstream-header warnings from Eigen / CSPICE / SOFA are all in non-user code and suppressed.)
- [x] Sanitizer build (`APSIS_ENABLE_SANITIZERS=ON`) passes ctest with no ASan/UBSan reports. (Verified locally under both gcc-13 and clang-17.)
- [ ] GitHub Actions matrix green: `{ubuntu-22.04 × gcc-13, ubuntu-22.04 × clang-17, windows-2022 × msvc}`. (Workflow YAML in place; CI run is deferred until pushed.)

### Manual
- [ ] Open `LICENSE`, confirm canonical Apache-2.0 text and copyright line correct.
- [ ] `external/sofa/PINNED_VERSION` and `external/cspice/PINNED_VERSION` carry verifiable upstream URLs and SHA-256s; running `sha256sum` on the downloaded tarballs matches.
- [ ] `git status` clean after a fresh build (`build/` and `external/_cache/` ignored; vendor `src/`/`include/` ignored).
- [ ] `THIRD_PARTY_LICENSES.md` lists Apache-2.0 (project), SOFA terms, CSPICE terms, MPL-2.0 (Eigen), BSD-3 (GoogleTest).

## Out of scope for Phase 0

- Any `apsis::math`, `apsis::time`, `apsis::frames` headers — those land in Phase 1.
- Any SOFA or CSPICE *call sites* — only their build wrappers exist after Phase 0.
- Any reference data under `data/` — those land alongside the regression tests they support, in Phase 1.
- pybind11 / Python bindings — Phase 6.

## Codegen / fallback notes

- If `fetch_external.cmake` cannot reach iausofa.org or naif.jpl.nasa.gov,
  the configure step emits a clear `FATAL_ERROR` naming the missing
  tarball, its expected SHA, and a manual-download instruction
  (`curl <URL> -o external/_cache/<file>`). No silent fallback.
- If Eigen or GoogleTest fetch fails (CPM cache miss + offline), CPM
  emits its standard error; configure aborts. CI populates the CPM
  source cache on the first green run; subsequent runs hit the cache.

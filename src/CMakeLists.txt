# ===========================================================================
#                  SeqAn - The Library for Sequence Analysis
# ===========================================================================
# File: /sandbox/h4nn3s/apps/lambda/CMakeLists.txt
#
# CMakeLists.txt file for lambda.
# ===========================================================================

cmake_minimum_required (VERSION 2.8.2)
project (lambda)
message (STATUS "Configuring lambda")

# ----------------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------------

# Search SeqAn and select dependencies.
set (SEQAN_FIND_DEPENDENCIES OpenMP ZLIB BZip2)
find_package(SeqAn REQUIRED)
find_package(CXX11 REQUIRED)
find_package(CXX14)

if (NOT CXX11_FOUND)
    message (FATAL_ERROR "  C++11 not found: Not building lambda.")
    return ()
endif (NOT CXX11_FOUND)

# Warn if OpenMP was not found.
if (NOT SEQAN_HAS_OPENMP)
    message (WARNING "WARNING WARNING WARNING\nWARNING: OpenMP not found. Lambda will be built without multi-threading! "
    "This is probably not what you want! Use GCC or a very, very recent version of Clang.\nWARNING WARNING WARNING")
endif (NOT SEQAN_HAS_OPENMP)

# Warn if Zlib was not found.
if (NOT SEQAN_HAS_ZLIB)
  message (WARNING "WARNING: Zlib not found. Building lambda without gzipped input and output.")
endif (NOT SEQAN_HAS_ZLIB)

# Warn if BZip2 was not found.
if (NOT SEQAN_HAS_BZIP2)
  message (WARNING "WARNING: BZip2 not found. Building lambda without bzipped input and output.")
endif (NOT SEQAN_HAS_BZIP2)

if (COMPILER_IS_CLANG)
    set (CXX11_CXX_FLAGS "${CXX11_CXX_FLAGS} -ftemplate-depth-1024")
endif (COMPILER_IS_CLANG)

if(CMAKE_COMPILER_IS_GNUCXX)
  if (491 GREATER _GCC_VERSION)
    message (FATAL_ERROR "Your GCC version is too old. Minimum version is GCC-4.9.1!")
    return ()
  endif(491 GREATER _GCC_VERSION)
endif(CMAKE_COMPILER_IS_GNUCXX)

# ----------------------------------------------------------------------------
# App-Level Configuration
# ----------------------------------------------------------------------------

set (SEQAN_APP_VERSION "0.9.1")

option (LAMBDA_FASTBUILD "Skip certain rarely used codepaths (speeds up build)." OFF)
if (LAMBDA_FASTBUILD)
    add_definitions (-DFASTBUILD=1)
    message (STATUS "FASTBUILD set, not all command line options available.")
endif (LAMBDA_FASTBUILD)

option (LAMBDA_NATIVE_BUILD "Architecture-specific optimizations, i.e. g++ -march=native." ON)
if (LAMBDA_NATIVE_BUILD)
    add_definitions (-DLAMBDA_NATIVE_BUILD=1)
    set (SEQAN_CXX_FLAGS "${SEQAN_CXX_FLAGS} -march=native")
    message (STATUS "NATIVE_BUILD set, binaries will be optimized for this CPU (and might not work on others).")
    message (STATUS "To deactivate, pass -DLAMBDA_NATIVE_BUILD=0 to cmake.")
endif (LAMBDA_NATIVE_BUILD)

if ((CMAKE_COMPILER_IS_GNUCXX OR COMPILER_IS_CLANG) AND NOT APPLE)
    option (LAMBDA_STATIC_BUILD "Include all libraries in the binaries." OFF)
    add_definitions (-DLAMBDA_STATIC_BUILD=1)
    if (LAMBDA_STATIC_BUILD)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
        if (CMAKE_COMPILER_IS_GNUCXX)
            set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
        else (CMAKE_COMPILER_IS_GNUCXX)
            set(CMAKE_EXE_LINKER_FLAGS "-static")
        endif (CMAKE_COMPILER_IS_GNUCXX)
        message (STATUS "STATIC_BUILD set, binaries built with all libs included.")
    endif (LAMBDA_STATIC_BUILD)
endif ()

option (LAMBDA_MMAPPED_DB "Use mmapped access to the database." ON)
if (LAMBDA_MMAPPED_DB)
    add_definitions (-DLAMBDA_MMAPPED_DB=1)
    message (STATUS "MMAPPED_DB set, using memory-mapped IO to access the database.")
    message (STATUS "To deactivate, pass -DLAMBDA_MMAPPED_DB=0 to cmake.")
endif (LAMBDA_MMAPPED_DB)

# ----------------------------------------------------------------------------
# Build Setup
# ----------------------------------------------------------------------------

# Enable global exception handler for all seqan apps.
set (SEQAN_DEFINITIONS "${SEQAN_DEFINITIONS} -DSEQAN_GLOBAL_EXCEPTION_HANDLER")

# Add include directories.
include_directories (${SEQAN_INCLUDE_DIRS})

# Add definitions set by find_package (SeqAn).
add_definitions (${SEQAN_DEFINITIONS})

# Add definitions set by the build system.
add_definitions (-DSEQAN_VERSION_STRING="${SEQAN_VERSION_STRING}")
add_definitions (-DSEQAN_REVISION="${SEQAN_REVISION}")
add_definitions (-DSEQAN_DATE="${SEQAN_DATE}")
add_definitions (-DSEQAN_APP_VERSION="${SEQAN_APP_VERSION}")

# Update the list of file names below if you add source files to your application.
add_executable (lambda lambda.cpp
                lambda.hpp
                misc.hpp
                match.hpp
                options.hpp
                alph.hpp
                holders.hpp)
add_executable (lambda_indexer lambda_indexer.cpp
                lambda_indexer.hpp
                options.hpp
                misc.hpp)

# Add dependencies found by find_package (SeqAn).
target_link_libraries (lambda ${SEQAN_LIBRARIES})
target_link_libraries (lambda_indexer ${SEQAN_LIBRARIES})

# Add CXX flags found by find_package (SeqAn).
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SEQAN_CXX_FLAGS} ${CXX11_CXX_FLAGS} -Wno-vla")

set(BUILD_SHARED_LIBS OFF)

# ----------------------------------------------------------------------------
# Installation
# ----------------------------------------------------------------------------

# Set variables for installing, depending on the selected build type.
if (NOT SEQAN_PREFIX_SHARE_DOC)
  seqan_setup_install_vars (lambda)
endif (NOT SEQAN_PREFIX_SHARE_DOC)

# Install lambda in ${PREFIX}/bin directory
install (TARGETS lambda lambda_indexer
         DESTINATION bin)

# Install non-binary files for the package to "." for app builds and
# ${PREFIX}/share/doc/lambda for SeqAn release builds.
install (FILES ../LICENSE-GPL3.rst
               ../README.rst
               ../COPYING.rst
         DESTINATION ${SEQAN_PREFIX_SHARE_DOC})
# install (FILES example/fasta1.fa
#               example/fasta2.fa
#         DESTINATION ${SEQAN_PREFIX_SHARE_DOC}/example)

# ----------------------------------------------------------------------------
# App Test
# ----------------------------------------------------------------------------

#seqan_add_app_test (lambda)

# ----------------------------------------------------------------------------
# CPack Install
# ----------------------------------------------------------------------------

if (SEQAN_BUILD_SYSTEM STREQUAL "APP:lambda")
  set (CPACK_PACKAGE_NAME "lambda")
  set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "lambda")
  set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name <your.email@example.net>")
  set (CPACK_PACKAGE_VENDOR "Your Name <your.email@example.net>")

  seqan_configure_cpack_app (lambda "lambda")
endif (SEQAN_BUILD_SYSTEM STREQUAL "APP:lambda")


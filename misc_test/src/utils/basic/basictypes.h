// Copyright 2014 asarcar Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

//! @file     basictypes.h
//! @brief    Minimal modification and adaptation of Craig Silverstein's
//!           basictypes.h found in Google's GPerfTools package.
//! @author   Arijit Sarcar <sarcar_a@yahoo.com>

// Copyright (c) 2005, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _BASE_UTILS_BASICTYPES_H_
#define _BASE_UTILS_BASICTYPES_H_

// C++ Standard Headers
// C Standard Headers
// Google Headers
// Arijit Sarcar: replaced with C++ headers: uint16_t available: ignoring sys/types.h
#include <cinttypes> // PRId8, PRIi16, PRIu32, PRIo64, PRIXPTR, SCNi32, ...
#include <cstdint>   // [u]int<N>_t & [U]INT<N>_<MAX|MIN> (ISO naming madness)

// To use this in an autoconf setting, make sure you run the following
// autoconf macros:
//    AC_HEADER_STDC              /* for stdint_h and inttypes_h */
//    AC_CHECK_TYPES([__int64])   /* defined in some windows platforms */

// Local Headers
// Arijit Sarcar TODO: include config.h: generate platform dependent
// defines e.g. "#define HAVE___ATTRIBUTE__ 1" based on the platform via cmake...
// #include "base_utils/config.h"

//! @addtogroup utils
//! @{

//! Umbrella namespace covering entire project
namespace asarcar {

// Standard typedefs
// All Google code is compiled with -funsigned-char to make "char"
// unsigned.  Google code therefore doesn't need a "uchar" type.
// TODO(csilvers): how do we make sure unsigned-char works on non-gcc systems?
typedef signed char         schar;
// int8_, int16_t, int32_t, int64_t, and intptr_ is available in cinttypes header
typedef int int128 __attribute__((mode(TI)));   

// Arijit Sarcar: C++11 defined all [un]signed integer types int<8..64>_t and intptr_t
// in <cstdint>. e.g. intptr_t, intmax_t, ...
// C++11 also defined [un]signed integers min & max values in <cstdint>
// e.g. INT16_MIN, INTPTR_MAX, INTMAX_MAX, and UINT64_MAX
// NOTE: unsigned types are DANGEROUS in loops and other arithmetical
// places. Reasons are:
// 1. C/C++ provide implicit type conversions between signed and unsigned values. 
// 2. If you mix signed and unsigned operands in an arithmetic operation, 
//    the signed operand is always converted to unsigned. 
// 3. In arithmetic expressions, operands whose type is smaller than (unsigned) int 
//    gets promoted to (unsigned) int. 
// 4. It is very easy to underflow the minimum value of an unsigned int (i.e. zero).

// Use the signed types unless your variable represents a bit
// pattern (eg a hash value) or you really need the extra bit.  Do NOT
// use 'unsigned' to express "this value should always be positive";
// use assertions for this.

// Define the "portable" printf and scanf macros, if they're not already there
// We just do something that works on many systems, and hope for the best

// Arijit Sarcar: C++11 defined format constants for std::fprintf family of functions
// and format constants for the std::fscanf family of function in <cinttypes>. 
// e.g. PRIX64, SCNx64, ...

// Arijit Sarcar: C++11 replaces DISALLOW_EVIL_CONSTRUCTORS by "delete" keyword
// Arijit Sarcar: C++11 replaces COMPILE_ASSERT by "static_assert"

#define arraysize(a)  (sizeof(a) / sizeof(*(a)))

#define OFFSETOF_MEMBER(strct, field)                                   \
   (reinterpret_cast<char*>(&reinterpret_cast<strct*>(16)->field) -     \
    reinterpret_cast<char*>(16))

#ifdef HAVE___ATTRIBUTE__
# define ATTRIBUTE_WEAK      __attribute__((weak))
# define ATTRIBUTE_NOINLINE  __attribute__((noinline))
#else
# define ATTRIBUTE_WEAK
# define ATTRIBUTE_NOINLINE
#endif

// Section attributes are supported for both ELF and Mach-O, but in
// very different ways.  Here's the API we provide:
// 1) ATTRIBUTE_SECTION: put this with the declaration of all functions
//    you want to be in the same linker section
// 2) DEFINE_ATTRIBUTE_SECTION_VARS: must be called once per unique
//    name.  You want to make sure this is executed before any
//    DECLARE_ATTRIBUTE_SECTION_VARS; the easiest way is to put them
//    in the same .cc file.  Put this call at the global level.
// 3) INIT_ATTRIBUTE_SECTION_VARS: you can scatter calls to this in
//    multiple places to help ensure execution before any
//    DECLARE_ATTRIBUTE_SECTION_VARS.  You must have at least one
//    DEFINE, but you can have many INITs.  Put each in its own scope.
// 4) DECLARE_ATTRIBUTE_SECTION_VARS: must be called before using
//    ATTRIBUTE_SECTION_START or ATTRIBUTE_SECTION_STOP on a name.
//    Put this call at the global level.
// 5) ATTRIBUTE_SECTION_START/ATTRIBUTE_SECTION_STOP: call this to say
//    where in memory a given section is.  All functions declared with
//    ATTRIBUTE_SECTION are guaranteed to be between START and STOP.

#if defined(HAVE___ATTRIBUTE__) && defined(__ELF__)
# define ATTRIBUTE_SECTION(name) __attribute__ ((section (#name)))

  // Weak section declaration to be used as a global declaration
  // for ATTRIBUTE_SECTION_START|STOP(name) to compile and link
  // even without functions with ATTRIBUTE_SECTION(name).
# define DECLARE_ATTRIBUTE_SECTION_VARS(name) \
    extern char __start_##name[] ATTRIBUTE_WEAK; \
    extern char __stop_##name[] ATTRIBUTE_WEAK
# define INIT_ATTRIBUTE_SECTION_VARS(name)     // no-op for ELF
# define DEFINE_ATTRIBUTE_SECTION_VARS(name)   // no-op for ELF

  // Return void* pointers to start/end of a section of code with functions
  // having ATTRIBUTE_SECTION(name), or 0 if no such function exists.
  // One must DECLARE_ATTRIBUTE_SECTION(name) for this to compile and link.
# define ATTRIBUTE_SECTION_START(name) (reinterpret_cast<void*>(__start_##name))
# define ATTRIBUTE_SECTION_STOP(name) (reinterpret_cast<void*>(__stop_##name))
# define HAVE_ATTRIBUTE_SECTION_START 1

#elif defined(HAVE___ATTRIBUTE__) && defined(__MACH__)
# define ATTRIBUTE_SECTION(name) __attribute__ ((section ("__DATA, " #name)))

#include <mach-o/getsect.h>
#include <mach-o/dyld.h>
class AssignAttributeStartEnd {
 public:
  AssignAttributeStartEnd(const char* name, char** pstart, char** pend) {
    // Find out what dynamic library name is defined in
    if (_dyld_present()) {
      for (int i = _dyld_image_count() - 1; i >= 0; --i) {
        const mach_header* hdr = _dyld_get_image_header(i);
        uint32_t len;
        *pstart = getsectdatafromheader(hdr, "__DATA", name, &len);
        if (*pstart) {   // NULL if not defined in this dynamic library
          *pstart += _dyld_get_image_vmaddr_slide(i);   // correct for reloc
          *pend = *pstart + len;
          return;
        }
      }
    }
    // If we get here, not defined in a dll at all.  See if defined statically.
    unsigned long len;    // don't ask me why this type isn't uint32_t too...
    *pstart = getsectdata("__DATA", name, &len);
    *pend = *pstart + len;
  }
};

#define DECLARE_ATTRIBUTE_SECTION_VARS(name)    \
  extern char* __start_##name;                  \
  extern char* __stop_##name

#define INIT_ATTRIBUTE_SECTION_VARS(name)               \
  DECLARE_ATTRIBUTE_SECTION_VARS(name);                 \
  static const AssignAttributeStartEnd __assign_##name( \
    #name, &__start_##name, &__stop_##name)

#define DEFINE_ATTRIBUTE_SECTION_VARS(name)     \
  char* __start_##name, *__stop_##name;         \
  INIT_ATTRIBUTE_SECTION_VARS(name)

# define ATTRIBUTE_SECTION_START(name) (reinterpret_cast<void*>(__start_##name))
# define ATTRIBUTE_SECTION_STOP(name) (reinterpret_cast<void*>(__stop_##name))
# define HAVE_ATTRIBUTE_SECTION_START 1

#else  // not HAVE___ATTRIBUTE__ && __ELF__, nor HAVE___ATTRIBUTE__ && __MACH__
# define ATTRIBUTE_SECTION(name)
# define DECLARE_ATTRIBUTE_SECTION_VARS(name)
# define INIT_ATTRIBUTE_SECTION_VARS(name)
# define DEFINE_ATTRIBUTE_SECTION_VARS(name)
# define ATTRIBUTE_SECTION_START(name) (reinterpret_cast<void*>(0))
# define ATTRIBUTE_SECTION_STOP(name) (reinterpret_cast<void*>(0))

#endif  // HAVE___ATTRIBUTE__ and __ELF__ or __MACH__

} // namespace asarcar
//! @} End of Doxygen asarcar

#endif  // _BASE_UTILS_BASICTYPES_H_

// Copyright (c) 1981-86 Robert A. Koeneke
// Copyright (c) 1987-94 James E. Wilson
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Sets up curses for the correct system.

// clang-format off

#ifdef _WIN32
  // this is defined in Windows and also in ncurses
  #undef KEY_EVENT
  #ifdef _MSVC_LANG
    // On Microsoft Visual Studio 2019, this constant also needs to
    // be undefined.  Also, we need to use the PDCurses library rather
    // than a system library, and it has a different include file name.
    #undef MOUSE_MOVED
    #include <curses.h>
  #else
    // ncursesw/ncurses.h only exposes the wide-character declarations
    // (wget_wch(), etc.) when NCURSES_WIDECHAR is already 1 by the time
    // it's included, which it otherwise derives from _XOPEN_SOURCE(_EXTENDED)
    // -- feature-test macros MinGW/UCRT64 don't set by default. We link
    // libncursesw.a (see CMakeLists.txt's MSYS/MINGW branch), which does
    // provide those functions, so force it on rather than relying on
    // _XOPEN_SOURCE plumbing.
    #define NCURSES_WIDECHAR 1
    #include <ncursesw/ncurses.h>
    #if NCURSES_WIDECHAR != 1
      #error "ncursesw (wide-character ncurses) is required on Windows/MinGW. Install mingw-w64-ucrt-x86_64-ncurses in an MSYS2 UCRT64 shell."
    #endif
  #endif
#elif __NetBSD__
  #include <curses.h>
#else
  #include <ncurses.h>
  #if !defined(NCURSES_WIDECHAR) || NCURSES_WIDECHAR != 1
    #error "ncursesw (wide-character ncurses) is required. Install the wide-character ncurses development package."
  #endif
#endif


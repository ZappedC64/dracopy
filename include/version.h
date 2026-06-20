#ifndef VERSION__H
#define VERSION__H

/* Fallback defaults so the program builds without passing -DDRA_VERNUM=...
   on the command line (command-line string-literal quoting is brittle on
   Windows). The Makefile's -D values still override these if supplied. */
#ifndef DRA_VERNUM
#define DRA_VERNUM "1.4rw"
#endif
#ifndef DRA_VERDATE
#define DRA_VERDATE "2026-06-18"
#endif

#if defined(KERBEROS)
#define DRA_VEREXTRA "-kerberos"
#elif defined(REU)
#define DRA_VEREXTRA "-reu"
#else
#define DRA_VEREXTRA ""
#endif

#define DRA_VER DRA_VERNUM DRA_VEREXTRA " " DRA_VERDATE

#endif

/*CXXR $Id$
 *CXXR
 *CXXR This file is part of CXXR, a project to refactor the R interpreter
 *CXXR into C++.  It may consist in whole or in part of program code and
 *CXXR documentation taken from the R project itself, incorporated into
 *CXXR CXXR (and possibly MODIFIED) under the terms of the GNU General Public
 *CXXR Licence.
 *CXXR 
 *CXXR CXXR is Copyright (C) 2008-11 Andrew R. Runnalls, subject to such other
 *CXXR copyrights and copyright restrictions as may be stated below.
 *CXXR 
 *CXXR CXXR is not part of the R project, and bugs and other issues should
 *CXXR not be reported via r-bugs or other R project channels; instead refer
 *CXXR to the CXXR website.
 *CXXR */

/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 1998--2007	    The R Development Core Team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 */

#ifndef R_BUFFER_UTILS
#define R_BUFFER_UTILS

#ifdef __cplusplus
extern "C" {
#endif

/* used in bind.c character.c deparse.c, printutils.c, saveload.c
   scan.c seq.c sprintf.c sysutils.c */

typedef struct {
 char *data;
 std::size_t bufsize;
 std::size_t defaultSize;
} R_StringBuffer;

/* code in ./memory.c : */
/* Note that R_StringBuffer *buf needs to be initialized before call */
void *R_AllocStringBuffer(std::size_t blen, R_StringBuffer *buf);
void R_FreeStringBuffer(R_StringBuffer *buf);
void R_FreeStringBufferL(R_StringBuffer *buf);

#ifdef __cplusplus
}
#endif

#endif

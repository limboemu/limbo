/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* A class used for error reporting in the MPEG classes */

#ifndef _MPEGERROR_H_
#define _MPEGERROR_H_

#include <stdio.h>
#include <stdarg.h>

class MPEGerror {
public:
    MPEGerror() {
        ClearError();
    }

    /* Set an error message */
    void SetError(const char *fmt, ...) {
        va_list ap;

        va_start(ap, fmt);
        vsprintf(errbuf, fmt, ap);
        va_end(ap);
        error = errbuf;
    }

    /* Find out if an error occurred */
    bool WasError(void) {
        return(error != NULL);
    }
    char *TheError(void) {
        return(error);
    }

    /* Clear any error message */
    void ClearError(void) {
        error = NULL;
    }

protected:
    char errbuf[512];
    char *error;
};

#endif /* _MPEGERROR_H_ */

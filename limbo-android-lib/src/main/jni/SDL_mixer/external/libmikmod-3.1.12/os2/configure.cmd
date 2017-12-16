/* REXX */

/*	MikMod sound library
	(c) 1998, 1999, 2000, 2001, 2002 Miodrag Vallat and others - see
	file AUTHORS for complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: configure.cmd,v 1.1.1.1 2004/06/01 12:16:17 raph Exp $

  Configuration script for libmikmod under OS/2

==============================================================================*/

ver_maj=3
ver_min=1
ver_micro=10
ver_beta=''

ECHO OFF
CALL main
ECHO ON
EXIT

/*
 *========== Helper functions
 */

yesno:
	ans=''
	DO WHILE ans=''
		SAY message" [y/n] "
		PULL ans
		ans=SUBSTR(ans,1,1)
		IF \((ans='N')|(ans='Y')) THEN
		DO
			SAY "Invalid answer. Please answer Y or N"
			ans=''
		END
	END
	RETURN ans
	EXIT

sed:
	IF LINES(fileout) THEN
	DO
		CALL LINEOUT fileout
		ERASE fileout
	END
	CALL LINEOUT fileout,,1
	linecount=0
	DO WHILE LINES(filein)
		line=LINEIN(filein)
		IF linecount\=0 THEN
		DO
			arro2=1
			DO WHILE (arro2\=0)
				arro1=POS('@',line)
				arro2=0
				IF (arro1\=0) THEN arro2=POS('@',line,arro1+1)
				IF (arro2\=0) THEN
				DO
					keyword=SUBSTR(line,arro1+1,arro2-arro1-1)
					SELECT
						WHEN keyword='AR' THEN keyword=ar
						WHEN keyword='ARFLAGS' THEN keyword=arflags
						WHEN keyword='CC' THEN keyword=cc
						WHEN keyword='CFLAGS' THEN keyword=cflags
						WHEN keyword='DEFNAME' THEN keyword=defname
						WHEN keyword='DLLNAME' THEN keyword=dllname
						WHEN keyword='DRIVER_OBJ' THEN keyword=driver_obj
						WHEN keyword='DRV_DART' THEN keyword=drv_dart
						WHEN keyword='DRV_OS2' THEN keyword=drv_os2
						WHEN keyword='IMPLIB' THEN keyword=implib
						WHEN keyword='LIB' THEN keyword=lib
						WHEN keyword='LIBMIKMOD_MAJOR_VERSION' THEN keyword=ver_maj
						WHEN keyword='LIBMIKMOD_MICRO_VERSION' THEN keyword=ver_micro
						WHEN keyword='LIBMIKMOD_MINOR_VERSION' THEN keyword=ver_min
						WHEN keyword='LIBNAME' THEN keyword=libname
						WHEN keyword='LIBS' THEN keyword=libs
						WHEN keyword='MAKE' THEN keyword=make
						WHEN keyword='ORULE' THEN keyword=orule
						WHEN keyword='DOES_NOT_HAVE_SIGNED' THEN keyword=''

						OTHERWISE NOP
					END
					line=SUBSTR(line,1,arro1-1)""keyword""SUBSTR(line,arro2+1,LENGTH(line)-arro2)
				END
			END
			/* convert forward slashes to backslashes for Watcom ? */
			IF convert="yes" THEN DO
				IF cc="wcc386" THEN DO
					arro1=1
					DO WHILE arro1\=0
						arro1=LASTPOS('/',line)
						IF (arro1\=0) THEN
							line=SUBSTR(line,1,arro1-1)"\"SUBSTR(line,arro1+1,LENGTH(line)-arro1)
					END
				END
			END
		END
		linecount=1
		CALL LINEOUT fileout, line
	END
	CALL LINEOUT fileout
	CALL LINEOUT filein
	RETURN

main:

/*
 *========== 1. Check the system and the compiler
 */

	libname="mikmod2.lib"
	dllname="mikmod2.dll"
	defname="mikmod2.def"

	build_dll=0
	lib=libname
	libs=""

	SAY "libmikmod/2 version "ver_maj"."ver_min"."ver_micro""ver_beta" configuration"
	SAY

/* OS/2
 * - MMPM/2 and DART drivers are available
 */

/* Don't check for fnmatch() */

	SAY
	SAY "Compiler..."
	SAY "You can compile libmikmod either with emx or with Watcom C. However, due to"
	SAY "the Unix nature of the library, emx is recommended."
	message="Do you want to use the emx compiler (recommended) ?"
	CALL yesno
	IF RESULT='Y' THEN
	DO
		SAY "Configuring for emx..."

		cc="gcc"
		cflags="-O2 -Zomf -Zmt -funroll-loops -ffast-math -fno-strength-reduce -Wall"

		SAY
		SAY "When building with emx, you can choose between building a static library, or"
		SAY "a DLL with an import library."
		message="Do you want to build a DLL (recommended) ?"
		CALL yesno
		IF RESULT='Y' THEN
		DO
			build_dll=1
			cflags=cflags" -Zdll"
			lib=dllname
		END
		ar="emxomfar"
		arflags="cr"
		make="make"
		orule="-o $@ -c"
		implib="emximp"
	END
	ELSE
	DO
		SAY "Configuring for Watcom C..."
		cc="wcc386"
		cflags="-5r -bt=os2 -fp5 -fpi87 -mf -oeatxh -w4 -zp8"
		ar="wlib"
		arflags="-b -c -n"
		make="wmake -ms"
		orule="-fo=$^@"
		implib=""
	END

/* "Checking" for include files */

	cflags=cflags" -DHAVE_FCNTL_H -DHAVE_LIMITS_H -DHAVE_UNISTD_H -DHAVE_SYS_IOCTL_H -DHAVE_SYS_TIME_H"

/*
 *========== 2. Ask the user for his/her choices
 */

/* Debug version */
	SAY
	SAY "Debugging..."
	message="Do you want a debug version ?"
	CALL yesno
	IF RESULT='Y' THEN
	DO
		cflags=cflags" -DMIKMOD_DEBUG"
		IF cc="gcc" THEN
			cflags=cflags" -g"
		ELSE IF cc="wcc386" THEN
			cflags=cflags" -d2"
	END
	ELSE
	DO
		IF cc="gcc" THEN
			cflags=cflags" -s -fomit-frame-pointer"
		ELSE IF cc="wcc386" THEN
			cflags=cflags" -d1"
	END

/* Drivers */
	SAY
	SAY "Drivers..."

	driver_obj=""

/* MMPM/2 driver */
	SAY "The MMPM/2 drivers will work with any OS/2 version starting from 2.1."
	SAY "If you're not running Warp 4, these drivers are recommended."
	message="Do you want the MMPM/2 drivers ?"
	CALL yesno
	IF RESULT='Y' THEN
	DO
		cflags=cflags" -DDRV_OS2"
		driver_obj=driver_obj" drv_os2.o"
		drv_os2="drv_os2 @106"
		libs="-lmmpm2"
	END
	ELSE
	DO
		drv_os2=""
	END

/* Dart driver */
	SAY "The DART (Direct Audio Real Time) driver will use less CPU time than the"
	SAY "standard MMPM/2 drivers, but will not work on OS/2 2.1 or 3.0."
	SAY "If you use Warp 4, this driver is recommended."
	message="Do you want the DART driver ?"
	CALL yesno
	IF RESULT='Y' THEN
	DO
		cflags=cflags" -DDRV_DART"
		driver_obj=driver_obj" drv_dart.o"
		drv_dart="drv_dart @105"
		IF libs="" THEN libs="-lmmpm2"
	END
	ELSE
	DO
		drv_dart=""
	END

/*
 *========== 3. Generate Makefiles
 */

	SAY

	filein ="Makefile.tmpl"
	fileout="..\libmikmod\Makefile"
	convert="yes"
	CALL sed

	filein ="..\include\mikmod.h.in"
	fileout="..\include\mikmod.h"
	convert="no"
	CALL sed

	filein ="..\include\mikmod.h.in"
	fileout="..\include\mikmod_build.h"
	convert="no"
	CALL sed

	filein =defname".in"
	fileout="..\libmikmod\"defname
	convert="no"
	CALL sed

	filein ="Makefile.os2"
	fileout="Make.cmd"
	convert="yes"
	CALL sed

/*
 *========== 4. Last notes
 */

	SAY
	SAY "Configuration is complete. libmikmod is ready to compile."
	SAY "Just enter 'make' at the command prompt..."
	SAY

	RETURN

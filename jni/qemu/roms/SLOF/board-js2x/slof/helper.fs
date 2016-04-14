\ *****************************************************************************
\ * Copyright (c) 2004, 2008 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

: slof-build-id  ( -- str len )
   flash-header 10 + a
;

: slof-revision s" 001" ;

: read-version-and-date
   flash-header 0= IF
   s"  " encode-string
   ELSE
   flash-header 10 + 10
   here swap rmove
   here 10
   s" , " $cat
   bdate2human $cat encode-string THEN
;

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


s" bulk" device-name


\ standard open firmare method


: open  true  ;

\ standard open firmare method


: close ;


\ -------------------------------------------------
\	Locals
\ ------------------------------------------------


8 chars alloc-mem VALUE setup-packet


\ --------------------------------------------------
\ signature --->4bytes offset --->0
\ tag       --->4bytes offset --->4
\ trans-len --->4bytes offset --->8
\ dir-flag  --->1byte  offset --->c
\ lun       --->1byte  offset --->d
\ comm-len  --->1byte  offset --->e
\ --------------------------------------------------


0 VALUE cbw-addr
: build-cbw ( address tag transfer-len direction lun command-len -- )
   5 pick TO cbw-addr  ( address tag transfer-len direction lun command-len )
   cbw-addr 0f erase   ( address tag transfer-len direction lun command-len )
   cbw-addr e + c!     ( address tag transfer-len direction lun )
   cbw-addr d + c!     ( address tag transfer-len direction )
   cbw-addr c + c!     ( address tag transfer-len )
   cbw-addr 8 + l!-le  ( address tag )
   cbw-addr 4 + l!-le  ( address )
   43425355 cbw-addr l!-le ( address )
   drop  ;


\ ---------------------------------------------------
\ signature --->4bytes offset --->0
\ tag       --->4bytes offset --->4
\ residue   --->4bytes offset --->8
\ status    --->1byte  offset --->c
\ ---------------------------------------------------


0 VALUE csw-addr
: analyze-csw ( address -- residue tag true|reason false )
   TO csw-addr
   csw-addr l@-le 53425355 =  IF
      csw-addr c + c@ dup 0=  IF ( reason )
         drop
         csw-addr 8 + l@-le ( residue )
         csw-addr 4 + l@-le ( residue tag ) \ command  block tag
         TRUE               ( residue tag TRUE )
      ELSE
         FALSE              ( reason FALSE )
      THEN
   ELSE
      FALSE                 ( FALSE )
   THEN
   csw-addr 0c erase
;

: bulk-reset-recovery-procedure ( bulk-out-endp bulk-in-endp usb-addr -- )
  s" bulk-reset-recovery-procedure" $call-parent
;

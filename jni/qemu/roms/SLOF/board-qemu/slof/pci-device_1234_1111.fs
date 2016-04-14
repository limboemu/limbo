\ *****************************************************************************
\ * Copyright (c) 2011 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

my-space pci-device-generic-setup

\ Defaults, overriden from qemu
d# 800 VALUE disp-width
d# 600 VALUE disp-height
d#   8 VALUE disp-depth

\ Determine base address
10 config-l@ translate-my-address f not AND VALUE fb-base

\ Fixed up later
-1 VALUE io-base

\ We support only one instance
false VALUE is-installed?

: vga-io-xlate ( port -- addr )
  io-base -1 = IF
    dup translate-my-address fff not and to io-base
  THEN
  io-base +
;

: vga-w! ( value port -- )
  vga-io-xlate rw!-le
;

: vga-w@ ( port -- value )
  vga-io-xlate rw@-le
;

: vga-b! ( value port -- )
  vga-io-xlate rb!
;

: vga-b@ ( port -- value )
  vga-io-xlate rb@
;

: vbe!	( value index -- )
  1ce vga-w! 1d0 vga-w!
;

: vbe@	( index -- value )
  1ce vga-w! 1d0 vga-w@
;

: color! ( r g b number -- ) 
   3c8 vga-b!
   rot 3c9 vga-b!
   swap 3c9 vga-b!
   3c9 vga-b!
;

: color@ ( number -- r g b ) 
   3c8 vga-b!
   3c9 vga-b@
   3c9 vga-b@
   3c9 vga-b@
;

: set-colors ( adr number #numbers -- )
   over 3c8 vga-b!
   swap DO
     rb@ 3c9 vga-b!
     rb@ 3c9 vga-b!
     rb@ 3c9 vga-b!
   LOOP
   3drop
;

: get-colors ( adr number #numbers -- )
   3drop
;

include graphics.fs

\ qemu fake VBE IO registers
0 CONSTANT VBE_DISPI_INDEX_ID
1 CONSTANT VBE_DISPI_INDEX_XRES
2 CONSTANT VBE_DISPI_INDEX_YRES
3 CONSTANT VBE_DISPI_INDEX_BPP
4 CONSTANT VBE_DISPI_INDEX_ENABLE
5 CONSTANT VBE_DISPI_INDEX_BANK
6 CONSTANT VBE_DISPI_INDEX_VIRT_WIDTH
7 CONSTANT VBE_DISPI_INDEX_VIRT_HEIGHT
8 CONSTANT VBE_DISPI_INDEX_X_OFFSET
9 CONSTANT VBE_DISPI_INDEX_Y_OFFSET
a CONSTANT VBE_DISPI_INDEX_NB

\ ENABLE register
00 CONSTANT VBE_DISPI_DISABLED
01 CONSTANT VBE_DISPI_ENABLED
02 CONSTANT VBE_DISPI_GETCAPS
20 CONSTANT VBE_DISPI_8BIT_DAC
40 CONSTANT VBE_DISPI_LFB_ENABLED
80 CONSTANT VBE_DISPI_NOCLEARMEM

: init-mode
  0 3c0 vga-b!
  VBE_DISPI_DISABLED VBE_DISPI_INDEX_ENABLE vbe!
  0 VBE_DISPI_INDEX_X_OFFSET vbe!
  0 VBE_DISPI_INDEX_Y_OFFSET vbe!
  disp-width VBE_DISPI_INDEX_XRES vbe!
  disp-height VBE_DISPI_INDEX_YRES vbe!
  disp-depth VBE_DISPI_INDEX_BPP vbe!
  VBE_DISPI_ENABLED VBE_DISPI_8BIT_DAC or VBE_DISPI_INDEX_ENABLE vbe!
  0 3c0 vga-b!
  20 3c0 vga-b!
;

: clear-screen
  fb-base disp-width disp-height disp-depth 7 + 8 / * * 0 rfill
;

: read-settings
  s" qemu,graphic-width" get-chosen IF
     decode-int to disp-width 2drop
  THEN
  s" qemu,graphic-height" get-chosen IF
     decode-int to disp-height 2drop
  THEN
  s" qemu,graphic-depth" get-chosen IF
     decode-int nip nip     
       dup 8 =
       over f = or
       over 10 = or
       over 20 = or IF
         to disp-depth
       ELSE
         ." Unsupported bit depth, using 8bpp " drop cr
       THEN
  THEN
;

: add-legacy-reg
  \ add legacy I/O Ports / Memory regions to assigned-addresses
  \ see PCI Bus Binding Revision 2.1 Section 7.
  s" reg" get-node get-property IF
    \ "reg" does not exist, create new
    encode-start
  ELSE
    \ "reg" does exist, copy it 
    encode-bytes
  THEN
  \ I/O Range 0x1ce-0x1d2
  my-space a1000000 or encode-int+ \ non-relocatable, aliased I/O space
  1ce encode-64+ 4 encode-64+ \ addr size
  \ I/O Range 0x3B0-0x3BB
  my-space a1000000 or encode-int+ \ non-relocatable, aliased I/O space
  3b0 encode-64+ c encode-64+ \ addr size
  \ I/O Range 0x3C0-0x3DF
  my-space a1000000 or encode-int+ \ non-relocatable, aliased I/O space
  3c0 encode-64+ 20 encode-64+ \ addr size
  \ Memory Range 0xA0000-0xBFFFF
  my-space a2000000 or encode-int+ \ non-relocatable, <1MB Memory space
  a0000 encode-64+ 20000 encode-64+ \ addr size
  s" reg" property \ store "reg" property
;

: setup-properties
   \ Shouldn't this be done from open ?
   disp-width encode-int s" width" property
   disp-height encode-int s" height" property
   disp-width disp-depth 7 + 8 / * encode-int s" linebytes" property
   disp-depth encode-int s" depth" property
   s" ISO8859-1" encode-string s" character-set" property \ i hope this is ok...
   \ add "device_type" property
   s" display" device-type
   s" qemu,std-vga" encode-string s" compatible" property
   \ XXX We don't create an "address" property because Linux doesn't know what
   \ to do with it for >32-bit
;

\ words for installation/removal, needed by is-install/is-remove, see display.fs
: display-remove ( -- ) 
;

: hcall-invert-screen ( -- )
    frame-buffer-adr frame-buffer-adr 3
    screen-height screen-width * screen-depth * /x /
    1 hv-logical-memop
    drop
;

: hcall-blink-screen ( -- )
    \ 32 msec delay for visually noticing the blink
    hcall-invert-screen 20 ms hcall-invert-screen
;

: display-install ( -- )
    is-installed? NOT IF
        ." Installing QEMU fb" cr
        fb-base to frame-buffer-adr
        clear-screen
        default-font 
        set-font
        disp-width disp-height
        disp-width char-width / disp-height char-height /
        disp-depth 7 + 8 /                      ( width height #lines #cols depth )
        fb-install
	['] hcall-invert-screen to invert-screen
	['] hcall-blink-screen to blink-screen
         true to is-installed?
    THEN
;

: set-alias
    s" screen" find-alias 0= IF
      \ no previous screen alias defined, define it...
      s" screen" get-node node>path set-alias
    ELSE
       drop
    THEN 
;


." qemu vga" cr

pci-master-enable
pci-mem-enable
pci-io-enable
add-legacy-reg
read-settings
init-mode
init-default-palette
setup-properties
' display-install is-install
' display-remove is-remove
set-alias

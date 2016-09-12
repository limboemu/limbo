[IFDEF] CONFIG_DRIVER_PCI

: pci-addr-encode ( addr.lo addr.mi addr.hi )
  rot >r swap >r 
  encode-int 
  r> encode-int encode+ 
  r> encode-int encode+
  ;
 
: pci-len-encode ( len.lo len.hi )
  encode-int 
  rot encode-int encode+ 
  ;

\ Get PCI physical address and size for configured BAR reg
: pci-bar>pci-addr ( bar-reg -- addr.lo addr.mid addr.hi size -1 | 0 )
  " assigned-addresses" active-package get-package-property 0= if
    begin
      decode-phys    \ ( reg prop prop-len phys.lo phys.mid phys.hi )
      dup ff and 6 pick = if
        >r >r >r rot drop
        decode-int drop decode-int
        -rot 2drop
        r> swap r> r> rot
        -1 exit
      else
        3drop
      then
      \ Drop the size as we don't need it
      decode-int drop decode-int drop
      dup 0=
    until
    3drop
    0 exit
  else
    0
  then
  ;

[THEN]

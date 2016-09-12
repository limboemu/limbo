\ *****************************************************************************
\ * Copyright (c) 2004, 2011 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

: dma-alloc ( ... size -- virt )
   \ ." dma-alloc called: " .s cr
   alloc-mem
;

: dma-free ( virt size -- )
   \ ." dma-free called: " .s cr
   free-mem
;

: dma-map-in ( ... virt size cacheable? -- devaddr )
   \ ." dma-map-in called: " .s cr
   2drop
;

: dma-map-out ( virt devaddr size -- )
   \ ." dma-map-out called: " .s cr
   2drop drop
;

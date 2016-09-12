\ tag: pseudo r-stack implementation for openbios
\ 
\ Copyright (C) 2016 Mark Cave-Ayland
\ 
\ See the file "COPYING" for further information about
\ the copyright and warranty status of this work.
\ 

\
\ Pseudo r-stack implementation for interpret mode
\

create prstack h# 20 cells allot
variable #prstack 0 #prstack !

: prstack-push  prstack #prstack @ cells + ! 1 #prstack +! ;
: prstack-pop   -1 #prstack +! prstack #prstack @ cells + @ ;

: >r state @ if ['] >r , exit then r> swap prstack-push >r ; immediate
: r> state @ if ['] r> , exit then r> prstack-pop swap >r ; immediate
: r@ state @ if ['] r@ , exit then r> prstack-pop dup prstack-push swap >r ; immediate

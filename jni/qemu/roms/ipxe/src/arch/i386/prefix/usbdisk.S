	.text
	.arch i386
	.section ".prefix", "awx", @progbits
	.code16
	.org 0

#include "mbr.S"

/* Partition table: ZIP-compatible partition 4, 64 heads, 32 sectors/track */
	.org 446
	.space 16
	.space 16
	.space 16
	.byte 0x80, 0x01, 0x01, 0x00
	.byte 0xeb, 0x3f, 0x20, 0x01
	.long 0x00000020
	.long 0x00000fe0

	.org 510
	.byte 0x55, 0xaa

/* Skip to start of partition */
	.org 32 * 512

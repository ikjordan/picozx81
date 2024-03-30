#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"
#include "dvi_config_defs.h"

.syntax unified
.cpu cortex-m0plus
.thumb

.macro decl_func_x name
.section .scratch_x.\name, "ax"
.global \name
.type \name,%function
.thumb_func
\name:
.endm

.macro decl_func_y name
.section .scratch_y.\name, "ax"
.global \name
.type \name,%function
.thumb_func
\name:
.endm

#define decl_func decl_func_x

// ----------------------------------------------------------------------------
// Fast Half res 1bpp black/white encoder

// Taking the encoder from DVI spec, with initial balance 0:
//
// - Encoding either 0x00 or 0xff will produce a running balance of -8, with
//   output symbol of 0x100 or 0x200
//
// - Subsequently encoding either 0x01 or 0xfe will return the balance to 0, with
//  output symbol of 0x1ff or 0x2ff
//
// So we can do 1bpp encode with a lookup of x coordinate LSB, and input
// colour bit. If we process pixels in even-sized blocks, only the colour
// lookup is needed.

// Encode 8 pixels @ 1bpp (using two table lookups)
// r3 contains lookup mask (preshifted)
// r8 contains pointer to encode table
// 2.125 cyc/pix
.macro tmds_double_1bpp_body shift_instr0 shamt0 shift_instr1 shamt1
	\shift_instr0 r4, r2, #\shamt0
	ands r4, r3             // Select 2 bits
	add r4, r8              // Get offset into lookup table
	ldmia r4, {r4, r5}      // Get values at the lookup table into r4 and r5
	\shift_instr1 r6, r2, #\shamt1
	ands r6, r3             // Get next 2 pixels
	add r6, r8
	ldmia r6, {r6, r7}      // Get values from look up table
	stmia r1!, {r4, r5, r6, r7} // Write out all values from LUT into output buffer and inc

.endm

// r0: input buffer (byte-aligned)
// r1: output buffer (word-aligned)
// r2: output pixel count
decl_func tmds_double_1bpp
	push {r4-r7, lr}        // Store scratch registers
	mov r7, r8
	push {r7}
	lsls r2, #1             // Double the output pixel count
	add r2, r1              // Now points to end of output buffer
	mov ip, r2              // ip is r12 - store end of output buffer
	adr r4, tmds_d1bpp_table // r4 contains pointer to look up table
	mov r8, r4              // .. as does r8
	// Mask: 2 bit index, 8 bytes per entry
	movs r3, #0x18          // bits 4 to 5 checked
	b 2f                    // Jump over for the first pass

1:
	ldrb r2, [r0]
	adds r0, #1
#if !DVI_1BPP_BIT_REVERSE
	tmds_double_1bpp_body lsls 3  lsls 1
	tmds_double_1bpp_body lsrs 1  lsrs 3
#else
	tmds_double_1bpp_body lsrs 3   lsrs 1
	tmds_double_1bpp_body lsls 1   lsls 3
#endif

2:
	cmp r1, ip              // Has all of the output data been generated
	blo 1b                  // If not, back to generate more

	pop {r7}                // Restore registers and return
	mov r8, r7
	pop {r4-r7, pc}

.align 2
tmds_d1bpp_table:
#if !DVI_1BPP_BIT_REVERSE
	.word 0x7fd00, 0x7fd00  // 0000
	.word 0xbfe00, 0x7fd00  // 0011
	.word 0x7fd00, 0xbfe00  // 1100
	.word 0xbfe00, 0xbfe00  // 1111
#else
	.word 0xbfe00, 0xbfe00  // 0000
	.word 0xbfe00, 0x7fd00  // 1100
	.word 0x7fd00, 0xbfe00  // 0011
	.word 0x7fd00, 0x7fd00  // 1111
#endif

// ----------------------------------------------------------------------------
// Half-resolution 2bpp encode (for 2bpp grayscale, or bitplaned RGB222)

// Even-x-position pixels are encoded as symbols with imbalance -4, and odd
// pixels with +4, so that we can mix-and-match our even/odd codewords and
// always get a properly balanced sequence:
//
// level 0: (05 -> 103), then (04 -> 1fc)  (decimal 5, 4)
// level 1: (50 -> 130), then (51 -> 1cf)  (decimal 80, 81)
// level 2: (af -> 230), then (ae -> 2cf)  (decimal 175, 174)
// level 3: (fa -> 203), then (fb -> 2fc)  (decimal 250, 251)
//
// These correspond to roughly 255 times (0, 1/3, 2/3, 1).
//
// Alternatively we could use symbols with 0 balance, which results in lower
// contrast but avoids the LSB bobble:
//
// level 0: (10 -> 1f0) always
// level 1: (5a -> 263) always
// level 2: (a5 -> 163) always
// level 3: (ef -> 2f0) always

// Table base pointer in r0. Input pixels in r2.
.macro tmds_double_2bpp_body shift_instr shamt rd
	\shift_instr \rd, r2, #\shamt
	ands \rd, r3
	ldr \rd, [r0, \rd]
.endm

// r0: input buffer (byte-aligned)
// r1: output buffer (word-aligned)
// r2: output pixel count
decl_func tmds_double_2bpp
	push {r4-r7, lr}
	mov r7, r8
	push {r7}
	mov r8, r0
	adr r0, tmds_d2bpp_table
	// Mask: 2-bit index into 4-byte entries.
	movs r3, #0x0c
	// Limit pointer: 1 word per 2 pixels
	lsls r2, #1
	add r2, r1
	mov ip, r2
	b 2f
1:
	mov r4, r8
	ldrb r2, [r4]
	adds r4, #1
	mov r8, r4
	tmds_double_2bpp_body lsls 2  r7
	tmds_double_2bpp_body lsrs 0  r6
	tmds_double_2bpp_body lsrs 2  r5
	tmds_double_2bpp_body lsrs 4  r4
	stmia r1!, {r4-r7}
2:
	cmp r1, ip
	blo 1b
	pop {r7}
	mov r8, r7
	pop {r4-r7, pc}

.align 2
tmds_d2bpp_table:
	.word 0xbf203 // 11, 11
	.word 0xb3e30 // 10, 10
	.word 0x73d30 // 01, 01
	.word 0x7f103 // 00, 00

// r0: start of tdms buffer for plane 1
// r1: pixel count to copy
decl_func tmds_clone
    push {lr}
    lsls r1, #2     // convert to 32 bit words
    adds r2, r0, r1 // Start of plane 2
    mov ip, r2      // End when get to plane 2
    adds r3, r2, r1 // Start of plane 3
    b 2f

1:
    ldm r0!, {r1}
    stm r2!, {r1}
    stm r3!, {r1}

2:
    cmp r0, ip
    blo 1b
    pop {pc}

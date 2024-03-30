#include "hardware/regs/addressmap.h"
#include "hardware/regs/sio.h"

.syntax unified
.cpu cortex-m0plus
.thumb

// r1 bit mask = 24
// r2 contains output buffer pointer
// r4 contains a byte of pixel data
// r5 contains TMDS LUT
// r6-r7 are for scratch + pixels
//
// Mask off the 2 bits we need and multiply by 8
.macro do_crumb pix_shift_instr pix_shamt
  \pix_shift_instr r6, r4, #\pix_shamt
  ands r6, r1
  add r6, r5
  ldmia r6, {r6, r7}
  stmia r2!, {r6-r7}
.endm

// r0 is the bitmap for this scanline
// r1 is colour buffer
// r2 is output TMDS buffer
// r3 is width in characters
// First stack argument is the color channel

.section .scratch_x.tmds_encode_screen, "ax"
.global tmds_encode_screen
.type tmds_encode_screen,%function
.thumb_func
tmds_encode_screen:

  push {r4-r7, lr}  // Store registers r4 to r10 + r14
  mov r4, r8
  mov r5, r9
  mov r6, r10
  push {r4-r6}

  lsls r3, #5       // (characters * 8 * 2 * 2)
  add r3, r2
  mov ip, r3        // ip alias for r12

  ldr r3, [sp, #32] // 8 words saved, so 32-byte offset to first stack argument
  lsls r3, #8       // * 256 - as each colour look up table is 256 bytes
  ldr r7, =attr_to_colour_chan_B
  adds r3, r7

  ldr r7, =palettised_2bpp_tables
  mov r9, r7
  mov r10, r1
  b 2f
1:
  mov r4, r10       // Attribute pointer to r4
  ldrb r1, [r4]     // Load the colour attribute
  adds r4, #1       // Increment the attribute pointer
  adds r1, r3       // Pointer to the entry in attr_to_colour_chan_X
  ldrb r1, [r1]     // Load the translated colour
  mov r10, r4       // store attribute pointer
  ldrb r4, [r0]     // Get pixel data
  lsls r5, r1, #5   // Multiply translated 4 bit colour by (4*8)
  add r5, r9        // Convert to offset into palettised_2bpp_tables
  movs r1, 24       // 00011000
  do_crumb lsrs 3   // Process the byte of pixel data in r4
  do_crumb lsrs 1
  do_crumb lsls 1
  do_crumb lsls 3
  adds r0, #1       // increment the pixel data pointer
2:
  cmp r2, ip        // r2 is output pointer incremented in do_crumb
  blo 1b

  pop {r4-r6}
  mov r8, r4
  mov r9, r5
  mov r10, r6
  pop {r4-r7, pc}   // Restore registers and put ret address in pc

//
// BORDER
//
// r0 is width in characters
// r1 is colour channel
// r2 is output TMDS buffer
// r3 is the colour attribute
//
.section .scratch_x.tmds_encode_border, "ax"
.global tmds_encode_border
.type tmds_encode_border,%function
.thumb_func
tmds_encode_border:

  push {r6-r7, lr}      // Store registers r4 to r10 + return address

  lsls r0, #5           // Work out the number of words (chars * 8 * 2 * 2)
  add r0, r2
  mov ip, r0            // ip is 1 past the last TDMS buffer address to write to

  lsls r1, #8           // * 256 to get into correct colour LUT
  ldr r7, =attr_to_colour_chan_B
  adds r1, r7

  adds r1, r3           // LUT index for colour attribute
  ldrb r1, [r1]         // Load the colour intensity nibble for the channel
                        // Will be in range 0 to 8
  ldr r7, =palettised_2bpp_tables
  lsls r1, #5           // * 8 * 4 (8 bytes per TMDS,
                        // 4 combinations of foreground and background)
  adds r1, r7           // r1 now points to the tdms LUT entry
  ldr r7, =(8*3)        // offset to both pixels as background
  adds r6, r1, r7       // move to the entry 1111
  ldmia r6, {r6, r7}    // Load the LUT entries into r6,r7
  b 4f

3:
  stmia r2!, {r6-r7}  // Write out the LUT entries
  stmia r2!, {r6-r7}  // ...
  stmia r2!, {r6-r7}  // ...
  stmia r2!, {r6-r7}  // ...
  stmia r2!, {r6-r7}  // ...
  stmia r2!, {r6-r7}  // ...
  stmia r2!, {r6-r7}  // ...
  stmia r2!, {r6-r7}  // ...

4:
  cmp r2, ip
  blo 3b

  pop {r6-r7, pc}    // Restore remaining registers set pc to return address


.section .scratch_x.palettised_2bpp_tables, "a"
.align 2
// 16 entries, each 32 bytes
// 8 bytes per entry - 4 bytes per pixel
palettised_2bpp_tables:
  // background, foreground = 00, 00 = 0
  .word 0x7f103, 0x7f103 // 0000
  .word 0x7f103, 0x7f103 // 1100
  .word 0x7f103, 0x7f103 // 0011
  .word 0x7f103, 0x7f103 // 1111
  // background, foreground = 00, 10 = 1
  .word 0x7f103, 0x7f103 // 0000
  .word 0x7f103, 0xb3e30 // 1100
  .word 0xb3e30, 0x7f103 // 0011
  .word 0xb3e30, 0xb3e30 // 1111
  // background, foreground = 00, 11 = 2
  .word 0x7f103, 0x7f103 // 0000
  .word 0x7f103, 0xbf203 // 1100
  .word 0xbf203, 0x7f103 // 0011
  .word 0xbf203, 0xbf203 // 1111
  // background, foreground = 10, 00 = 3
  .word 0xb3e30, 0xb3e30 // 0000
  .word 0xb3e30, 0x7f103 // 1100
  .word 0x7f103, 0xb3e30 // 0011
  .word 0x7f103, 0x7f103 // 1111
  // background, foreground = 10, 10 = 4
  .word 0xb3e30, 0xb3e30 // 0000
  .word 0xb3e30, 0xb3e30 // 1100
  .word 0xb3e30, 0xb3e30 // 0011
  .word 0xb3e30, 0xb3e30 // 1111
  // background, foreground = 10, 11 = 5
  .word 0xb3e30, 0xb3e30 // 0000
  .word 0xb3e30, 0xbf203 // 1100
  .word 0xbf203, 0xb3e30 // 0011
  .word 0xbf203, 0xbf203 // 1111
  // background, foreground = 11, 00 = 6
  .word 0xbf203, 0xbf203 // 0000
  .word 0xbf203, 0x7f103 // 1100
  .word 0x7f103, 0xbf203 // 0011
  .word 0x7f103, 0x7f103 // 1111
  // background, foreground = 11, 10 = 7
  .word 0xbf203, 0xbf203 // 0000
  .word 0xbf203, 0xb3e30 // 1100
  .word 0xb3e30, 0xbf203 // 0011
  .word 0xb3e30, 0xb3e30 // 1111
  // background, foreground = 11, 11 = 8
  .word 0xbf203, 0xbf203 // 0000
  .word 0xbf203, 0xbf203 // 1100
  .word 0xbf203, 0xbf203 // 0011
  .word 0xbf203, 0xbf203 // 1111

.align 1
attr_to_colour_chan_B:
    // B in bit 0. Bright in bit 3
    // 0000
	.byte 0x00 // B 0000
	.byte 0x01 // B 0001
	.byte 0x00 // B 0010
	.byte 0x01 // B 0011
	.byte 0x00 // B 0100
	.byte 0x01 // B 0101
	.byte 0x00 // B 0110
	.byte 0x01 // B 0111
	.byte 0x00 // B 1000
	.byte 0x02 // B 1001
	.byte 0x00 // B 1010
	.byte 0x02 // B 1011
	.byte 0x00 // B 1100
	.byte 0x02 // B 1101
	.byte 0x00 // B 1110
	.byte 0x02 // B 1111

    // 0001
	.byte 0x03 // B 0000
	.byte 0x04 // B 0001
	.byte 0x03 // B 0010
	.byte 0x04 // B 0011
	.byte 0x03 // B 0100
	.byte 0x04 // B 0101
	.byte 0x03 // B 0110
	.byte 0x04 // B 0111
	.byte 0x03 // B 1000
	.byte 0x05 // B 1001
	.byte 0x03 // B 1010
	.byte 0x05 // B 1011
	.byte 0x03 // B 1100
	.byte 0x05 // B 1101
	.byte 0x03 // B 1110
	.byte 0x05 // B 1111

    // 0010
	.byte 0x00 // B 0000
	.byte 0x01 // B 0001
	.byte 0x00 // B 0010
	.byte 0x01 // B 0011
	.byte 0x00 // B 0100
	.byte 0x01 // B 0101
	.byte 0x00 // B 0110
	.byte 0x01 // B 0111
	.byte 0x00 // B 1000
	.byte 0x02 // B 1001
	.byte 0x00 // B 1010
	.byte 0x02 // B 1011
	.byte 0x00 // B 1100
	.byte 0x02 // B 1101
	.byte 0x00 // B 1110
	.byte 0x02 // B 1111

    // 0011
	.byte 0x03 // B 0000
	.byte 0x04 // B 0001
	.byte 0x03 // B 0010
	.byte 0x04 // B 0011
	.byte 0x03 // B 0100
	.byte 0x04 // B 0101
	.byte 0x03 // B 0110
	.byte 0x04 // B 0111
	.byte 0x03 // B 1000
	.byte 0x05 // B 1001
	.byte 0x03 // B 1010
	.byte 0x05 // B 1011
	.byte 0x03 // B 1100
	.byte 0x05 // B 1101
	.byte 0x03 // B 1110
	.byte 0x05 // B 1111

    // 0100
	.byte 0x00 // B 0000
	.byte 0x01 // B 0001
	.byte 0x00 // B 0010
	.byte 0x01 // B 0011
	.byte 0x00 // B 0100
	.byte 0x01 // B 0101
	.byte 0x00 // B 0110
	.byte 0x01 // B 0111
	.byte 0x00 // B 1000
	.byte 0x02 // B 1001
	.byte 0x00 // B 1010
	.byte 0x02 // B 1011
	.byte 0x00 // B 1100
	.byte 0x02 // B 1101
	.byte 0x00 // B 1110
	.byte 0x02 // B 1111

    // 0101
	.byte 0x03 // B 0000
	.byte 0x04 // B 0001
	.byte 0x03 // B 0010
	.byte 0x04 // B 0011
	.byte 0x03 // B 0100
	.byte 0x04 // B 0101
	.byte 0x03 // B 0110
	.byte 0x04 // B 0111
	.byte 0x03 // B 1000
	.byte 0x05 // B 1001
	.byte 0x03 // B 1010
	.byte 0x05 // B 1011
	.byte 0x03 // B 1100
	.byte 0x05 // B 1101
	.byte 0x03 // B 1110
	.byte 0x05 // B 1111

    // 0110
	.byte 0x00 // B 0000
	.byte 0x01 // B 0001
	.byte 0x00 // B 0010
	.byte 0x01 // B 0011
	.byte 0x00 // B 0100
	.byte 0x01 // B 0101
	.byte 0x00 // B 0110
	.byte 0x01 // B 0111
	.byte 0x00 // B 1000
	.byte 0x02 // B 1001
	.byte 0x00 // B 1010
	.byte 0x02 // B 1011
	.byte 0x00 // B 1100
	.byte 0x02 // B 1101
	.byte 0x00 // B 1110
	.byte 0x02 // B 1111

    // 0111
	.byte 0x03 // B 0000
	.byte 0x04 // B 0001
	.byte 0x03 // B 0010
	.byte 0x04 // B 0011
	.byte 0x03 // B 0100
	.byte 0x04 // B 0101
	.byte 0x03 // B 0110
	.byte 0x04 // B 0111
	.byte 0x03 // B 1000
	.byte 0x05 // B 1001
	.byte 0x03 // B 1010
	.byte 0x05 // B 1011
	.byte 0x03 // B 1100
	.byte 0x05 // B 1101
	.byte 0x03 // B 1110
	.byte 0x05 // B 1111

    // 1000
	.byte 0x00 // B 0000
	.byte 0x01 // B 0001
	.byte 0x00 // B 0010
	.byte 0x01 // B 0011
	.byte 0x00 // B 0100
	.byte 0x01 // B 0101
	.byte 0x00 // B 0110
	.byte 0x01 // B 0111
	.byte 0x00 // B 1000
	.byte 0x02 // B 1001
	.byte 0x00 // B 1010
	.byte 0x02 // B 1011
	.byte 0x00 // B 1100
	.byte 0x02 // B 1101
	.byte 0x00 // B 1110
	.byte 0x02 // B 1111

    // 1001
	.byte 0x06 // B 0000
	.byte 0x07 // B 0001
	.byte 0x06 // B 0010
	.byte 0x07 // B 0011
	.byte 0x06 // B 0100
	.byte 0x07 // B 0101
	.byte 0x06 // B 0110
	.byte 0x07 // B 0111
	.byte 0x06 // B 1000
	.byte 0x08 // B 1001
	.byte 0x06 // B 1010
	.byte 0x08 // B 1011
	.byte 0x06 // B 1100
	.byte 0x08 // B 1101
	.byte 0x06 // B 1110
	.byte 0x08 // B 1111

    // 1010
	.byte 0x00 // B 0000
	.byte 0x01 // B 0001
	.byte 0x00 // B 0010
	.byte 0x01 // B 0011
	.byte 0x00 // B 0100
	.byte 0x01 // B 0101
	.byte 0x00 // B 0110
	.byte 0x01 // B 0111
	.byte 0x00 // B 1000
	.byte 0x02 // B 1001
	.byte 0x00 // B 1010
	.byte 0x02 // B 1011
	.byte 0x00 // B 1100
	.byte 0x02 // B 1101
	.byte 0x00 // B 1110
	.byte 0x02 // B 1111

    // 1011
	.byte 0x06 // B 0000
	.byte 0x07 // B 0001
	.byte 0x06 // B 0010
	.byte 0x07 // B 0011
	.byte 0x06 // B 0100
	.byte 0x07 // B 0101
	.byte 0x06 // B 0110
	.byte 0x07 // B 0111
	.byte 0x06 // B 1000
	.byte 0x08 // B 1001
	.byte 0x06 // B 1010
	.byte 0x08 // B 1011
	.byte 0x06 // B 1100
	.byte 0x08 // B 1101
	.byte 0x06 // B 1110
	.byte 0x08 // B 1111

    // 1100
	.byte 0x00 // B 0000
	.byte 0x01 // B 0001
	.byte 0x00 // B 0010
	.byte 0x01 // B 0011
	.byte 0x00 // B 0100
	.byte 0x01 // B 0101
	.byte 0x00 // B 0110
	.byte 0x01 // B 0111
	.byte 0x00 // B 1000
	.byte 0x02 // B 1001
	.byte 0x00 // B 1010
	.byte 0x02 // B 1011
	.byte 0x00 // B 1100
	.byte 0x02 // B 1101
	.byte 0x00 // B 1110
	.byte 0x02 // B 1111

    // 1101
	.byte 0x06 // B 0000
	.byte 0x07 // B 0001
	.byte 0x06 // B 0010
	.byte 0x07 // B 0011
	.byte 0x06 // B 0100
	.byte 0x07 // B 0101
	.byte 0x06 // B 0110
	.byte 0x07 // B 0111
	.byte 0x06 // B 1000
	.byte 0x08 // B 1001
	.byte 0x06 // B 1010
	.byte 0x08 // B 1011
	.byte 0x06 // B 1100
	.byte 0x08 // B 1101
	.byte 0x06 // B 1110
	.byte 0x08 // B 1111

    // 1110
	.byte 0x00 // B 0000
	.byte 0x01 // B 0001
	.byte 0x00 // B 0010
	.byte 0x01 // B 0011
	.byte 0x00 // B 0100
	.byte 0x01 // B 0101
	.byte 0x00 // B 0110
	.byte 0x01 // B 0111
	.byte 0x00 // B 1000
	.byte 0x02 // B 1001
	.byte 0x00 // B 1010
	.byte 0x02 // B 1011
	.byte 0x00 // B 1100
	.byte 0x02 // B 1101
	.byte 0x00 // B 1110
	.byte 0x02 // B 1111

    // 1111
	.byte 0x06 // B 0000
	.byte 0x07 // B 0001
	.byte 0x06 // B 0010
	.byte 0x07 // B 0011
	.byte 0x06 // B 0100
	.byte 0x07 // B 0101
	.byte 0x06 // B 0110
	.byte 0x07 // B 0111
	.byte 0x06 // B 1000
	.byte 0x08 // B 1001
	.byte 0x06 // B 1010
	.byte 0x08 // B 1011
	.byte 0x06 // B 1100
	.byte 0x08 // B 1101
	.byte 0x06 // B 1110
	.byte 0x08 // B 1111

.align 1
attr_to_colour_chan_G:
    // G in bit 2. Bright in bit 3
    // 0000
	.byte 0x00 // G 0000
	.byte 0x00 // G 0001
	.byte 0x00 // G 0010
	.byte 0x00 // G 0011
	.byte 0x01 // G 0100
	.byte 0x01 // G 0101
	.byte 0x01 // G 0110
	.byte 0x01 // G 0111
	.byte 0x00 // G 1000
	.byte 0x00 // G 1001
	.byte 0x00 // G 1010
	.byte 0x00 // G 1011
	.byte 0x02 // G 1100
	.byte 0x02 // G 1101
	.byte 0x02 // G 1110
	.byte 0x02 // G 1111

    // 0001
	.byte 0x00 // G 0000
	.byte 0x00 // G 0001
	.byte 0x00 // G 0010
	.byte 0x00 // G 0011
	.byte 0x01 // G 0100
	.byte 0x01 // G 0101
	.byte 0x01 // G 0110
	.byte 0x01 // G 0111
	.byte 0x00 // G 1000
	.byte 0x00 // G 1001
	.byte 0x00 // G 1010
	.byte 0x00 // G 1011
	.byte 0x02 // G 1100
	.byte 0x02 // G 1101
	.byte 0x02 // G 1110
	.byte 0x02 // G 1111

    // 0010
	.byte 0x00 // G 0000
	.byte 0x00 // G 0001
	.byte 0x00 // G 0010
	.byte 0x00 // G 0011
	.byte 0x01 // G 0100
	.byte 0x01 // G 0101
	.byte 0x01 // G 0110
	.byte 0x01 // G 0111
	.byte 0x00 // G 1000
	.byte 0x00 // G 1001
	.byte 0x00 // G 1010
	.byte 0x00 // G 1011
	.byte 0x02 // G 1100
	.byte 0x02 // G 1101
	.byte 0x02 // G 1110
	.byte 0x02 // G 1111

    // 0011
	.byte 0x00 // G 0000
	.byte 0x00 // G 0001
	.byte 0x00 // G 0010
	.byte 0x00 // G 0011
	.byte 0x01 // G 0100
	.byte 0x01 // G 0101
	.byte 0x01 // G 0110
	.byte 0x01 // G 0111
	.byte 0x00 // G 1000
	.byte 0x00 // G 1001
	.byte 0x00 // G 1010
	.byte 0x00 // G 1011
	.byte 0x02 // G 1100
	.byte 0x02 // G 1101
	.byte 0x02 // G 1110
	.byte 0x02 // G 1111

    // 0100
	.byte 0x03 // G 0000
	.byte 0x03 // G 0001
	.byte 0x03 // G 0010
	.byte 0x03 // G 0011
	.byte 0x04 // G 0100
	.byte 0x04 // G 0101
	.byte 0x04 // G 0110
	.byte 0x04 // G 0111
	.byte 0x03 // G 1000
	.byte 0x03 // G 1001
	.byte 0x03 // G 1010
	.byte 0x03 // G 1011
	.byte 0x05 // G 1100
	.byte 0x05 // G 1101
	.byte 0x05 // G 1110
	.byte 0x05 // G 1111

    // 0101
	.byte 0x03 // G 0000
	.byte 0x03 // G 0001
	.byte 0x03 // G 0010
	.byte 0x03 // G 0011
	.byte 0x04 // G 0100
	.byte 0x04 // G 0101
	.byte 0x04 // G 0110
	.byte 0x04 // G 0111
	.byte 0x03 // G 1000
	.byte 0x03 // G 1001
	.byte 0x03 // G 1010
	.byte 0x03 // G 1011
	.byte 0x05 // G 1100
	.byte 0x05 // G 1101
	.byte 0x05 // G 1110
	.byte 0x05 // G 1111

    // 0110
	.byte 0x03 // G 0000
	.byte 0x03 // G 0001
	.byte 0x03 // G 0010
	.byte 0x03 // G 0011
	.byte 0x04 // G 0100
	.byte 0x04 // G 0101
	.byte 0x04 // G 0110
	.byte 0x04 // G 0111
	.byte 0x03 // G 1000
	.byte 0x03 // G 1001
	.byte 0x03 // G 1010
	.byte 0x03 // G 1011
	.byte 0x05 // G 1100
	.byte 0x05 // G 1101
	.byte 0x05 // G 1110
	.byte 0x05 // G 1111

    // 0111
	.byte 0x03 // G 0000
	.byte 0x03 // G 0001
	.byte 0x03 // G 0010
	.byte 0x03 // G 0011
	.byte 0x04 // G 0100
	.byte 0x04 // G 0101
	.byte 0x04 // G 0110
	.byte 0x04 // G 0111
	.byte 0x03 // G 1000
	.byte 0x03 // G 1001
	.byte 0x03 // G 1010
	.byte 0x03 // G 1011
	.byte 0x05 // G 1100
	.byte 0x05 // G 1101
	.byte 0x05 // G 1110
	.byte 0x05 // G 1111

    // 1000
	.byte 0x00 // G 0000
	.byte 0x00 // G 0001
	.byte 0x00 // G 0010
	.byte 0x00 // G 0011
	.byte 0x01 // G 0100
	.byte 0x01 // G 0101
	.byte 0x01 // G 0110
	.byte 0x01 // G 0111
	.byte 0x00 // G 1000
	.byte 0x00 // G 1001
	.byte 0x00 // G 1010
	.byte 0x00 // G 1011
	.byte 0x02 // G 1100
	.byte 0x02 // G 1101
	.byte 0x02 // G 1110
	.byte 0x02 // G 1111

    // 1001
	.byte 0x00 // G 0000
	.byte 0x00 // G 0001
	.byte 0x00 // G 0010
	.byte 0x00 // G 0011
	.byte 0x01 // G 0100
	.byte 0x01 // G 0101
	.byte 0x01 // G 0110
	.byte 0x01 // G 0111
	.byte 0x00 // G 1000
	.byte 0x00 // G 1001
	.byte 0x00 // G 1010
	.byte 0x00 // G 1011
	.byte 0x02 // G 1100
	.byte 0x02 // G 1101
	.byte 0x02 // G 1110
	.byte 0x02 // G 1111

    // 1010
	.byte 0x00 // G 0000
	.byte 0x00 // G 0001
	.byte 0x00 // G 0010
	.byte 0x00 // G 0011
	.byte 0x01 // G 0100
	.byte 0x01 // G 0101
	.byte 0x01 // G 0110
	.byte 0x01 // G 0111
	.byte 0x00 // G 1000
	.byte 0x00 // G 1001
	.byte 0x00 // G 1010
	.byte 0x00 // G 1011
	.byte 0x02 // G 1100
	.byte 0x02 // G 1101
	.byte 0x02 // G 1110
	.byte 0x02 // G 1111

    // 1011
	.byte 0x00 // G 0000
	.byte 0x00 // G 0001
	.byte 0x00 // G 0010
	.byte 0x00 // G 0011
	.byte 0x01 // G 0100
	.byte 0x01 // G 0101
	.byte 0x01 // G 0110
	.byte 0x01 // G 0111
	.byte 0x00 // G 1000
	.byte 0x00 // G 1001
	.byte 0x00 // G 1010
	.byte 0x00 // G 1011
	.byte 0x02 // G 1100
	.byte 0x02 // G 1101
	.byte 0x02 // G 1110
	.byte 0x02 // G 1111

    // 1100
	.byte 0x06 // G 0000
	.byte 0x06 // G 0001
	.byte 0x06 // G 0010
	.byte 0x06 // G 0011
	.byte 0x07 // G 0100
	.byte 0x07 // G 0101
	.byte 0x07 // G 0110
	.byte 0x07 // G 0111
	.byte 0x06 // G 1000
	.byte 0x06 // G 1001
	.byte 0x06 // G 1010
	.byte 0x06 // G 1011
	.byte 0x08 // G 1100
	.byte 0x08 // G 1101
	.byte 0x08 // G 1110
	.byte 0x08 // G 1111

    // 1101
	.byte 0x06 // G 0000
	.byte 0x06 // G 0001
	.byte 0x06 // G 0010
	.byte 0x06 // G 0011
	.byte 0x07 // G 0100
	.byte 0x07 // G 0101
	.byte 0x07 // G 0110
	.byte 0x07 // G 0111
	.byte 0x06 // G 1000
	.byte 0x06 // G 1001
	.byte 0x06 // G 1010
	.byte 0x06 // G 1011
	.byte 0x08 // G 1100
	.byte 0x08 // G 1101
	.byte 0x08 // G 1110
	.byte 0x08 // G 1111

    // 1110
	.byte 0x06 // G 0000
	.byte 0x06 // G 0001
	.byte 0x06 // G 0010
	.byte 0x06 // G 0011
	.byte 0x07 // G 0100
	.byte 0x07 // G 0101
	.byte 0x07 // G 0110
	.byte 0x07 // G 0111
	.byte 0x06 // G 1000
	.byte 0x06 // G 1001
	.byte 0x06 // G 1010
	.byte 0x06 // G 1011
	.byte 0x08 // G 1100
	.byte 0x08 // G 1101
	.byte 0x08 // G 1110
	.byte 0x08 // G 1111

    // 1111
	.byte 0x06 // G 0000
	.byte 0x06 // G 0001
	.byte 0x06 // G 0010
	.byte 0x06 // G 0011
	.byte 0x07 // G 0100
	.byte 0x07 // G 0101
	.byte 0x07 // G 0110
	.byte 0x07 // G 0111
	.byte 0x06 // G 1000
	.byte 0x06 // G 1001
	.byte 0x06 // G 1010
	.byte 0x06 // G 1011
	.byte 0x08 // G 1100
	.byte 0x08 // G 1101
	.byte 0x08 // G 1110
	.byte 0x08 // G 1111

.align 1
attr_to_colour_chan_R:
    // R in bit 1. Bright in bit 3
    // 0000
	.byte 0x00 // R 0000
	.byte 0x00 // R 0001
	.byte 0x01 // R 0010
	.byte 0x01 // R 0011
	.byte 0x00 // R 0100
	.byte 0x00 // R 0101
	.byte 0x01 // R 0110
	.byte 0x01 // R 0111
	.byte 0x00 // R 1000
	.byte 0x00 // R 1001
	.byte 0x02 // R 1010
	.byte 0x02 // R 1011
	.byte 0x00 // R 1100
	.byte 0x00 // R 1101
	.byte 0x02 // R 1110
	.byte 0x02 // R 1111

    // 0001
	.byte 0x00 // R 0000
	.byte 0x00 // R 0001
	.byte 0x01 // R 0010
	.byte 0x01 // R 0011
	.byte 0x00 // R 0100
	.byte 0x00 // R 0101
	.byte 0x01 // R 0110
	.byte 0x01 // R 0111
	.byte 0x00 // R 1000
	.byte 0x00 // R 1001
	.byte 0x02 // R 1010
	.byte 0x02 // R 1011
	.byte 0x00 // R 1100
	.byte 0x00 // R 1101
	.byte 0x02 // R 1110
	.byte 0x02 // R 1111

    // 0010
	.byte 0x03 // R 0000
	.byte 0x03 // R 0001
	.byte 0x04 // R 0010
	.byte 0x04 // R 0011
	.byte 0x03 // R 0100
	.byte 0x03 // R 0101
	.byte 0x04 // R 0110
	.byte 0x04 // R 0111
	.byte 0x03 // R 1000
	.byte 0x03 // R 1001
	.byte 0x05 // R 1010
	.byte 0x05 // R 1011
	.byte 0x03 // R 1100
	.byte 0x03 // R 1101
	.byte 0x05 // R 1110
	.byte 0x05 // R 1111

    // 0011
	.byte 0x03 // R 0000
	.byte 0x03 // R 0001
	.byte 0x04 // R 0010
	.byte 0x04 // R 0011
	.byte 0x03 // R 0100
	.byte 0x03 // R 0101
	.byte 0x04 // R 0110
	.byte 0x04 // R 0111
	.byte 0x03 // R 1000
	.byte 0x03 // R 1001
	.byte 0x05 // R 1010
	.byte 0x05 // R 1011
	.byte 0x03 // R 1100
	.byte 0x03 // R 1101
	.byte 0x05 // R 1110
	.byte 0x05 // R 1111

    // 0100
	.byte 0x00 // R 0000
	.byte 0x00 // R 0001
	.byte 0x01 // R 0010
	.byte 0x01 // R 0011
	.byte 0x00 // R 0100
	.byte 0x00 // R 0101
	.byte 0x01 // R 0110
	.byte 0x01 // R 0111
	.byte 0x00 // R 1000
	.byte 0x00 // R 1001
	.byte 0x02 // R 1010
	.byte 0x02 // R 1011
	.byte 0x00 // R 1100
	.byte 0x00 // R 1101
	.byte 0x02 // R 1110
	.byte 0x02 // R 1111

    // 0101
	.byte 0x00 // R 0000
	.byte 0x00 // R 0001
	.byte 0x01 // R 0010
	.byte 0x01 // R 0011
	.byte 0x00 // R 0100
	.byte 0x00 // R 0101
	.byte 0x01 // R 0110
	.byte 0x01 // R 0111
	.byte 0x00 // R 1000
	.byte 0x00 // R 1001
	.byte 0x02 // R 1010
	.byte 0x02 // R 1011
	.byte 0x00 // R 1100
	.byte 0x00 // R 1101
	.byte 0x02 // R 1110
	.byte 0x02 // R 1111

    // 0110
	.byte 0x03 // R 0000
	.byte 0x03 // R 0001
	.byte 0x04 // R 0010
	.byte 0x04 // R 0011
	.byte 0x03 // R 0100
	.byte 0x03 // R 0101
	.byte 0x04 // R 0110
	.byte 0x04 // R 0111
	.byte 0x03 // R 1000
	.byte 0x03 // R 1001
	.byte 0x05 // R 1010
	.byte 0x05 // R 1011
	.byte 0x03 // R 1100
	.byte 0x03 // R 1101
	.byte 0x05 // R 1110
	.byte 0x05 // R 1111

    // 0111
	.byte 0x03 // R 0000
	.byte 0x03 // R 0001
	.byte 0x04 // R 0010
	.byte 0x04 // R 0011
	.byte 0x03 // R 0100
	.byte 0x03 // R 0101
	.byte 0x04 // R 0110
	.byte 0x04 // R 0111
	.byte 0x03 // R 1000
	.byte 0x03 // R 1001
	.byte 0x05 // R 1010
	.byte 0x05 // R 1011
	.byte 0x03 // R 1100
	.byte 0x03 // R 1101
	.byte 0x05 // R 1110
	.byte 0x05 // R 1111

    // 1000
	.byte 0x00 // R 0000
	.byte 0x00 // R 0001
	.byte 0x01 // R 0010
	.byte 0x01 // R 0011
	.byte 0x00 // R 0100
	.byte 0x00 // R 0101
	.byte 0x01 // R 0110
	.byte 0x01 // R 0111
	.byte 0x00 // R 1000
	.byte 0x00 // R 1001
	.byte 0x02 // R 1010
	.byte 0x02 // R 1011
	.byte 0x00 // R 1100
	.byte 0x00 // R 1101
	.byte 0x02 // R 1110
	.byte 0x02 // R 1111

    // 1001
	.byte 0x00 // R 0000
	.byte 0x00 // R 0001
	.byte 0x01 // R 0010
	.byte 0x01 // R 0011
	.byte 0x00 // R 0100
	.byte 0x00 // R 0101
	.byte 0x01 // R 0110
	.byte 0x01 // R 0111
	.byte 0x00 // R 1000
	.byte 0x00 // R 1001
	.byte 0x02 // R 1010
	.byte 0x02 // R 1011
	.byte 0x00 // R 1100
	.byte 0x00 // R 1101
	.byte 0x02 // R 1110
	.byte 0x02 // R 1111

    // 1010
	.byte 0x06 // R 0000
	.byte 0x06 // R 0001
	.byte 0x07 // R 0010
	.byte 0x07 // R 0011
	.byte 0x06 // R 0100
	.byte 0x06 // R 0101
	.byte 0x07 // R 0110
	.byte 0x07 // R 0111
	.byte 0x06 // R 1000
	.byte 0x06 // R 1001
	.byte 0x08 // R 1010
	.byte 0x08 // R 1011
	.byte 0x06 // R 1100
	.byte 0x06 // R 1101
	.byte 0x08 // R 1110
	.byte 0x08 // R 1111

    // 1011
	.byte 0x06 // R 0000
	.byte 0x06 // R 0001
	.byte 0x07 // R 0010
	.byte 0x07 // R 0011
	.byte 0x06 // R 0100
	.byte 0x06 // R 0101
	.byte 0x07 // R 0110
	.byte 0x07 // R 0111
	.byte 0x06 // R 1000
	.byte 0x06 // R 1001
	.byte 0x08 // R 1010
	.byte 0x08 // R 1011
	.byte 0x06 // R 1100
	.byte 0x06 // R 1101
	.byte 0x08 // R 1110
	.byte 0x08 // R 1111

    // 1100
	.byte 0x00 // R 0000
	.byte 0x00 // R 0001
	.byte 0x01 // R 0010
	.byte 0x01 // R 0011
	.byte 0x00 // R 0100
	.byte 0x00 // R 0101
	.byte 0x01 // R 0110
	.byte 0x01 // R 0111
	.byte 0x00 // R 1000
	.byte 0x00 // R 1001
	.byte 0x02 // R 1010
	.byte 0x02 // R 1011
	.byte 0x00 // R 1100
	.byte 0x00 // R 1101
	.byte 0x02 // R 1110
	.byte 0x02 // R 1111

    // 1101
	.byte 0x00 // R 0000
	.byte 0x00 // R 0001
	.byte 0x01 // R 0010
	.byte 0x01 // R 0011
	.byte 0x00 // R 0100
	.byte 0x00 // R 0101
	.byte 0x01 // R 0110
	.byte 0x01 // R 0111
	.byte 0x00 // R 1000
	.byte 0x00 // R 1001
	.byte 0x02 // R 1010
	.byte 0x02 // R 1011
	.byte 0x00 // R 1100
	.byte 0x00 // R 1101
	.byte 0x02 // R 1110
	.byte 0x02 // R 1111

    // 1110
	.byte 0x06 // R 0000
	.byte 0x06 // R 0001
	.byte 0x07 // R 0010
	.byte 0x07 // R 0011
	.byte 0x06 // R 0100
	.byte 0x06 // R 0101
	.byte 0x07 // R 0110
	.byte 0x07 // R 0111
	.byte 0x06 // R 1000
	.byte 0x06 // R 1001
	.byte 0x08 // R 1010
	.byte 0x08 // R 1011
	.byte 0x06 // R 1100
	.byte 0x06 // R 1101
	.byte 0x08 // R 1110
	.byte 0x08 // R 1111

    // 1111
	.byte 0x06 // R 0000
	.byte 0x06 // R 0001
	.byte 0x07 // R 0010
	.byte 0x07 // R 0011
	.byte 0x06 // R 0100
	.byte 0x06 // R 0101
	.byte 0x07 // R 0110
	.byte 0x07 // R 0111
	.byte 0x06 // R 1000
	.byte 0x06 // R 1001
	.byte 0x08 // R 1010
	.byte 0x08 // R 1011
	.byte 0x06 // R 1100
	.byte 0x06 // R 1101
	.byte 0x08 // R 1110
	.byte 0x08 // R 1111

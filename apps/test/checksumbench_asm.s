; # fn_calc_checksum_asm(const uint8_t *data, uint16_t len)
; #
; # Amiga m68k C interop: arguments occupy four-byte stack slots. Immediately
; # after JSR, 4(sp) is the data pointer; the uint16_t length is zero-extended
; # into the slot at 8(sp), with its low word at 10(sp). The uint8_t result is
; # returned in the low byte of D0.
; #
; # D0, D1, and A0 are caller-saved and are used here for the accumulator, byte
; # count, and data pointer. D2 is callee-saved by the Amiga C ABI, so it must be
; # saved and restored around this routine or the C caller's state may be
; # corrupted.
; #
; # For each byte, add it to the 16-bit accumulator, then fold the high byte back
; # into the low byte. This is the same end-around carry order used by the C
; # fn_checksum_fold() implementation; changing the order changes the checksum.

        .globl  _fn_calc_checksum_asm
_fn_calc_checksum_asm:
        ; # Fetch arguments before changing SP. AT&T syntax has source first and
        ; # prefixes registers with %.
        move.l  4(%sp),%a0
        move.w  10(%sp),%d1
        # Preserve the callee-saved scratch register used by the fold.
        movem.l %d2,-(%sp)
        # D0 is the 16-bit running accumulator; D1 counts remaining bytes.
        moveq   #0,%d0
        tst.w   %d1
        beq.s   .done
.loop:
        ; # Load one unsigned byte and add it to the accumulator.
        moveq   #0,%d2
        move.b  (%a0)+,%d2
        add.w   %d2,%d0
        ; # Fold the high byte into the low byte, matching fn_checksum_fold().
        move.w  %d0,%d2
        lsr.w   #8,%d2
        andi.w  #0x00ff,%d0
        add.w   %d2,%d0
        subq.w  #1,%d1
        bne.s   .loop
.done:
        # Restore D2 while leaving D0 unchanged for the return.
        movem.l (%sp)+,%d2
        rts

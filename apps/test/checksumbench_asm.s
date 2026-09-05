        .globl  _fn_calc_checksum_asm
_fn_calc_checksum_asm:
        move.l  8(%sp),%a0
        move.w  12(%sp),%d1
        moveq   #0,%d0
        tst.w   %d1
        beq.s   .done
.loop:
        moveq   #0,%d2
        move.b  (%a0)+,%d2
        add.w   %d2,%d0
        move.w  %d0,%d2
        lsr.w   #8,%d2
        andi.w  #0x00ff,%d0
        add.w   %d2,%d0
        subq.w  #1,%d1
        bne.s   .loop
.done:
        rts

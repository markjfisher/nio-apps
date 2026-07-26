        .export _config_nio_bbc_host_get
        .export _config_nio_bbc_host_set
        .export _config_nio_bbc_host_clear
        .export _config_nio_bbc_entry_get
        .export _config_nio_bbc_entry_set
        .export _config_nio_bbc_slot_get
        .export _config_nio_bbc_slot_set
        .export _config_nio_bbc_mapping_get
        .export _config_nio_bbc_mapping_set
        .export _config_nio_bbc_mapping_clear

        .import popa, popax
        .import _config_nio_xram_begin
        .import _config_nio_xram_end
        .importzp ptr1, ptr2, tmp1, tmp2, tmp3, tmp4

XRAM_BANK       = 7
XRAM_BASE_HI    = $80

HOST_MAX        = 16
HOST_SIZE       = $A1

SLOT_MAX        = 8
SLOT_BASE_LO    = $10
SLOT_BASE_HI    = $8A
SLOT_SIZE       = $A6

MAPPING_MAX     = 8
MAPPING_BASE_LO = $40
MAPPING_BASE_HI = $8F
MAPPING_SIZE    = 3

ENTRY_MAX       = 14
ENTRY_BASE_LO   = $58
ENTRY_BASE_HI   = $8F
ENTRY_SIZE      = $25

        .code

return0:
        lda     #0
        tax
        rts

return1:
        lda     #1
        ldx     #0
        rts

select_xram:
        lda     #XRAM_BANK
        jmp     _config_nio_xram_begin

copy_xram_to_ram:
        lda     tmp1
        beq     copy_done
        jsr     select_xram
        ldy     #0
@loop:  lda     (ptr1),y
        sta     (ptr2),y
        iny
        cpy     tmp1
        bne     @loop
        jmp     _config_nio_xram_end

copy_ram_to_xram:
        lda     tmp1
        beq     copy_done
        jsr     select_xram
        ldy     #0
@loop:  lda     (ptr2),y
        sta     (ptr1),y
        iny
        cpy     tmp1
        bne     @loop
        jmp     _config_nio_xram_end

copy_done:
        rts

host_ptr:
        sta     tmp2
        lda     #0
        sta     ptr1
        lda     #XRAM_BASE_HI
        sta     ptr1+1
        lda     tmp2
        beq     @done
@loop:  clc
        lda     ptr1
        adc     #HOST_SIZE
        sta     ptr1
        bcc     @next
        inc     ptr1+1
@next:  dec     tmp2
        bne     @loop
@done:
        rts

slot_ptr:
        sta     tmp2
        lda     #SLOT_BASE_LO
        sta     ptr1
        lda     #SLOT_BASE_HI
        sta     ptr1+1
        lda     tmp2
        beq     @done
@loop:  clc
        lda     ptr1
        adc     #SLOT_SIZE
        sta     ptr1
        bcc     @next
        inc     ptr1+1
@next:  dec     tmp2
        bne     @loop
@done:  rts

mapping_ptr:
        sta     tmp2
        lda     #MAPPING_BASE_LO
        sta     ptr1
        lda     #MAPPING_BASE_HI
        sta     ptr1+1
        lda     tmp2
        beq     @done
@loop:  clc
        lda     ptr1
        adc     #MAPPING_SIZE
        sta     ptr1
        bcc     @next
        inc     ptr1+1
@next:  dec     tmp2
        bne     @loop
@done:  rts

entry_ptr:
        sta     tmp2
        lda     #ENTRY_BASE_LO
        sta     ptr1
        lda     #ENTRY_BASE_HI
        sta     ptr1+1
        lda     tmp2
        beq     @done
@loop:  clc
        lda     ptr1
        adc     #ENTRY_SIZE
        sta     ptr1
        bcc     @next
        inc     ptr1+1
@next:  dec     tmp2
        bne     @loop
@done:  rts

; int config_nio_bbc_host_get(uint8_t index, char *buf, uint16_t cap)
_config_nio_bbc_host_get:
        sta     tmp1            ; cap low
        stx     tmp2            ; cap high
        jsr     popax
        sta     ptr2
        stx     ptr2+1
        jsr     popa
        sta     tmp3            ; index

        lda     ptr2
        ora     ptr2+1
        beq     @bad
        lda     tmp1
        ora     tmp2
        beq     @bad

        ldy     #0
        tya
        sta     (ptr2),y

        lda     tmp3
        cmp     #HOST_MAX
        bcs     @bad

        lda     tmp2
        bne     @cap_host
        lda     tmp1
        cmp     #HOST_SIZE+1
        bcc     @cap_ok
@cap_host:
        lda     #HOST_SIZE
        sta     tmp1
@cap_ok:
        lda     tmp1
        sec
        sbc     #1
        sta     tmp4            ; bytes to read

        lda     tmp3
        jsr     host_ptr
        lda     tmp4
        sta     tmp1
        jsr     copy_xram_to_ram

        ldy     tmp4
        lda     #0
        sta     (ptr2),y
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_host_set(uint8_t index, const char *value)
_config_nio_bbc_host_set:
        sta     ptr2
        stx     ptr2+1
        jsr     popa
        cmp     #HOST_MAX
        bcs     @bad
        jsr     host_ptr

        jsr     select_xram
        lda     ptr2
        ora     ptr2+1
        beq     @term
        ldy     #0
@loop:  cpy     #HOST_SIZE-1
        beq     @term
        lda     (ptr2),y
        sta     (ptr1),y
        beq     @done
        iny
        bne     @loop
@term:  lda     #0
        sta     (ptr1),y
@done:  jsr     _config_nio_xram_end
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_host_clear(uint8_t index)
_config_nio_bbc_host_clear:
        cmp     #HOST_MAX
        bcs     @bad
        jsr     host_ptr
        jsr     select_xram
        ldy     #0
        tya
        sta     (ptr1),y
        jsr     _config_nio_xram_end
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_entry_get(uint8_t index, config_nio_entry_t *entry)
_config_nio_bbc_entry_get:
        sta     ptr2
        stx     ptr2+1
        jsr     popa
        sta     tmp3
        lda     ptr2
        ora     ptr2+1
        beq     @bad
        lda     tmp3
        cmp     #ENTRY_MAX
        bcs     @bad
        jsr     entry_ptr
        lda     #ENTRY_SIZE
        sta     tmp1
        jsr     copy_xram_to_ram
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_entry_set(uint8_t index, const config_nio_entry_t *entry)
_config_nio_bbc_entry_set:
        sta     ptr2
        stx     ptr2+1
        jsr     popa
        sta     tmp3
        lda     ptr2
        ora     ptr2+1
        beq     @bad
        lda     tmp3
        cmp     #ENTRY_MAX
        bcs     @bad
        jsr     entry_ptr
        lda     #ENTRY_SIZE
        sta     tmp1
        jsr     copy_ram_to_xram
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_slot_get(uint8_t index, config_nio_slot_t *slot)
_config_nio_bbc_slot_get:
        sta     ptr2
        stx     ptr2+1
        jsr     popa
        sta     tmp3
        lda     ptr2
        ora     ptr2+1
        beq     @bad
        lda     tmp3
        cmp     #SLOT_MAX
        bcs     @bad
        jsr     slot_ptr
        lda     #SLOT_SIZE
        sta     tmp1
        jsr     copy_xram_to_ram
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_slot_set(uint8_t index, const config_nio_slot_t *slot)
_config_nio_bbc_slot_set:
        sta     ptr2
        stx     ptr2+1
        jsr     popa
        sta     tmp3
        lda     ptr2
        ora     ptr2+1
        beq     @bad
        lda     tmp3
        cmp     #SLOT_MAX
        bcs     @bad
        jsr     slot_ptr
        lda     #SLOT_SIZE
        sta     tmp1
        jsr     copy_ram_to_xram
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_mapping_get(uint8_t unit, config_nio_mapping_t *mapping)
_config_nio_bbc_mapping_get:
        sta     ptr2
        stx     ptr2+1
        jsr     popa
        sta     tmp3
        lda     ptr2
        ora     ptr2+1
        beq     @bad
        lda     tmp3
        cmp     #MAPPING_MAX
        bcs     @bad
        jsr     mapping_ptr
        lda     #MAPPING_SIZE
        sta     tmp1
        jsr     copy_xram_to_ram
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_mapping_set(uint8_t unit, const config_nio_mapping_t *mapping)
_config_nio_bbc_mapping_set:
        sta     ptr2
        stx     ptr2+1
        jsr     popa
        sta     tmp3
        lda     ptr2
        ora     ptr2+1
        beq     @bad
        lda     tmp3
        cmp     #MAPPING_MAX
        bcs     @bad
        jsr     mapping_ptr
        lda     #MAPPING_SIZE
        sta     tmp1
        jsr     copy_ram_to_xram
        jmp     return1
@bad:  jmp     return0

; int config_nio_bbc_mapping_clear(uint8_t unit)
_config_nio_bbc_mapping_clear:
        cmp     #MAPPING_MAX
        bcs     @bad
        jsr     mapping_ptr
        jsr     select_xram
        ldy     #MAPPING_SIZE-1
        lda     #0
@loop:  sta     (ptr1),y
        dey
        bpl     @loop
        jsr     _config_nio_xram_end
        jmp     return1
@bad:  jmp     return0

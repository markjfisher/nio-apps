        .export _config_nio_bbc_build_mappings
        .export _config_nio_bbc_parse_hosts

        .import popax
        .import _config_nio_xram_begin
        .import _config_nio_xram_end
        .import _config_nio_appstore_buf
        .import _config_nio_store_buf
        .import _config_nio_bbc_parse_len
        .import _config_nio_bbc_line_len
        .import _config_nio_bbc_parse_finish
        .importzp ptr1, ptr2, ptr3, tmp1, tmp2, tmp3, tmp4

XRAM_BANK       = 7
HOST_MAX        = 16
HOST_SIZE       = $A1
HOST_BASE_LO    = $00
HOST_BASE_HI    = $80
MAPPING_MAX     = 8
MAPPING_BASE_LO = $40
MAPPING_BASE_HI = $8F
MAPPING_SIZE    = 3

        .code

; uint16_t config_nio_bbc_build_mappings(uint8_t *buf, uint16_t cap)
_config_nio_bbc_build_mappings:
        sta     tmp1                    ; cap low, enough for current BBC buf
        jsr     popax
        sta     ptr2                    ; output
        stx     ptr2+1
        lda     ptr2
        ora     ptr2+1
        beq     @zero
        lda     tmp1
        cmp     #7
        bcc     @zero

        lda     #MAPPING_BASE_LO
        sta     ptr1
        lda     #MAPPING_BASE_HI
        sta     ptr1+1
        lda     #0
        sta     tmp2                    ; output offset
        sta     tmp3                    ; unit

        lda     #XRAM_BANK
        jsr     _config_nio_xram_begin
@row:
        ldy     #0
        lda     (ptr1),y                ; valid
        beq     @next
        lda     tmp2
        clc
        adc     #7
        cmp     tmp1
        bcc     @room
        beq     @room
        jmp     @done
@room:
        ldy     #0
        lda     tmp3
        clc
        adc     #'0'
        jsr     put_out
        lda     #$09
        jsr     put_out
        ldy     #1
        lda     (ptr1),y                ; slot
        clc
        adc     #'0'
        jsr     put_out
        lda     #$09
        jsr     put_out
        lda     #'r'
        jsr     put_out
        ldy     #2
        lda     (ptr1),y                ; readonly
        beq     @rw
        lda     #'o'
        bne     @mode
@rw:   lda     #'w'
@mode: jsr     put_out
        lda     #$0A
        jsr     put_out
@next:
        clc
        lda     ptr1
        adc     #MAPPING_SIZE
        sta     ptr1
        bcc     @unit_next
        inc     ptr1+1
@unit_next:
        inc     tmp3
        lda     tmp3
        cmp     #MAPPING_MAX
        bcc     @row
@done:
        jsr     _config_nio_xram_end
        lda     tmp2
        ldx     #0
        rts
@zero:
        lda     #0
        tax
        rts

put_out:
        ldy     tmp2
        sta     (ptr2),y
        inc     tmp2
        rts

; uint16_t config_nio_bbc_parse_hosts(config_nio_state_t *state)
_config_nio_bbc_parse_hosts:
        sta     ptr3
        stx     ptr3+1
        lda     ptr3
        ora     ptr3+1
        bne     @state_ok
        lda     #0
        tax
        rts

@state_ok:
        lda     _config_nio_bbc_line_len
        sta     tmp4                    ; current line length
        lda     _config_nio_bbc_parse_len
        sta     tmp1                    ; bytes available in input chunk
        lda     #<_config_nio_appstore_buf
        sta     ptr2
        lda     #>_config_nio_appstore_buf
        sta     ptr2+1
        lda     #0
        sta     tmp2                    ; input offset

@parse_loop:
        lda     tmp2
        cmp     tmp1
        bcs     @parse_done
        ldy     tmp2
        lda     (ptr2),y
        inc     tmp2
        cmp     #$0A
        beq     @newline
        cmp     #$0D
        beq     @newline
        ldy     tmp4
        cpy     #HOST_SIZE-1
        bcs     @parse_loop
        sta     _config_nio_store_buf,y
        inc     tmp4
        bne     @parse_loop

@newline:
        lda     tmp4
        beq     @parse_loop
        jsr     commit_host_line
        lda     #0
        sta     tmp4
        jmp     @parse_loop

@parse_done:
        lda     _config_nio_bbc_parse_finish
        beq     @return_len
        lda     tmp4
        beq     @return_len
        jsr     commit_host_line
        lda     #0
        sta     tmp4

@return_len:
        lda     tmp4
        sta     _config_nio_bbc_line_len
        ldx     #0
        rts

commit_host_line:
        ldy     #0
        lda     (ptr3),y                ; state->host_count
        cmp     #HOST_MAX
        bcc     @room
        rts

@room:
        ldy     tmp4
        lda     #0
        sta     _config_nio_store_buf,y
        ldy     #0
        lda     (ptr3),y
        jsr     host_ptr

        lda     #XRAM_BANK
        jsr     _config_nio_xram_begin
        ldy     #0
@copy:
        lda     _config_nio_store_buf,y
        sta     (ptr1),y
        beq     @copied
        iny
        cpy     #HOST_SIZE
        bne     @copy
        dey
        lda     #0
        sta     (ptr1),y
@copied:
        jsr     _config_nio_xram_end

        ldy     #0
        lda     (ptr3),y
        clc
        adc     #1
        sta     (ptr3),y
        rts

host_ptr:
        sta     tmp3
        lda     #HOST_BASE_LO
        sta     ptr1
        lda     #HOST_BASE_HI
        sta     ptr1+1
        lda     tmp3
        beq     @done
@loop:
        clc
        lda     ptr1
        adc     #HOST_SIZE
        sta     ptr1
        bcc     @next
        inc     ptr1+1
@next:
        dec     tmp3
        bne     @loop
@done:
        rts

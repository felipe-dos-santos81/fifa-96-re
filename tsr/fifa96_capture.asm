; tsr/fifa96_capture.asm — FIFA96 P0 capture rig (16-bit .COM TSR).
;   nasm -f bin tsr/fifa96_capture.asm -o build/FIFACAP.COM
;
; Task 3a: resident install skeleton — COM1 sink, framed records, INT-21
; pre/post chain seams, INT-60/INT-1 passthrough. FILE/CODEC emission,
; heartbeat counting, the patch pass and pre-state save/restore land in
; Task 3b/4; the seams below are the only places they attach.
        org 0x100
        bits 16

COM1_TX   equ 0x3F8
COM1_LSR  equ 0x3FD

T_HEADER  equ 0x01
T_FILE    equ 0x02
T_CODEC   equ 0x03
T_HB      equ 0x04
T_SKIP    equ 0x05
T_END     equ 0x06
T_POK     equ 0x07

start:  jmp install

; ───────────────────────────── resident data ─────────────────────────────
old_int21  dd 0
old_int60  dd 0
old_int1   dd 0
patchpend  db 0
pending    db 0FFh
filecount  dd 0
seq_file   dw 0
seq_codec  dw 0
seq_hb     dw 0
rec_buf    times 160 db 0          ; payload scratch (max 160 B)
pre_ah     db 0                    ; pre-state snapshot (Task 3b fills)
pre_bx     dw 0
pre_cx     dw 0
pre_ds     dw 0
pre_dx     dw 0
g_flags    dw 0                    ; handler result flags, captured at .after

; ───────────────────────────── resident code ─────────────────────────────
; putc — transmit AL to COM1 (0x3F8) after a bounded THRE spin on LSR bit5.
; Drops the byte silently on timeout (surfaces downstream as a seq gap).
; Preserves AX, BX, CX, DX.
putc:
        push ax
        push bx
        push cx
        push dx
        mov  bl, al
        mov  cx, 0xFFFF
.try:
        mov  dx, COM1_LSR
        in   al, dx
        test al, 0x20
        jnz  .ok
        loop .try
        jmp  .out
.ok:
        mov  dx, COM1_TX
        mov  al, bl
        out  dx, al
.out:
        pop  dx
        pop  cx
        pop  bx
        pop  ax
        ret

; putw — transmit AX high byte then low byte. Preserves AX.
putw:
        push ax
        mov  al, ah
        call putc
        pop  ax
        call putc
        ret

; send_frame — AL=type, DS:SI=payload, CX=len.
; Frame: type:u8, seq:u16, len:u16, payload[len].
; FILE/CODEC/HEARTBEAT draw from and increment a per-type counter; all other
; types emit seq 0 and are untracked. Preserves all registers.
send_frame:
        push ax
        push bx
        push bp
        push cx
        push dx
        push si
        push di
        push ds
        push es

        xor  bx, bx                    ; seq value (0 for untracked types)
        xor  bp, bp                    ; address of tracked slot (0 = untracked)
        cmp  al, T_FILE
        jne  .codec
        mov  bp, seq_file
        jmp  .load
.codec:
        cmp  al, T_CODEC
        jne  .hb
        mov  bp, seq_codec
        jmp  .load
.hb:
        cmp  al, T_HB
        jne  .emit
        mov  bp, seq_hb
.load:
        mov  bx, [cs:bp]
.emit:
        call putc                      ; type
        mov  al, bl
        call putc                      ; seq lo
        mov  al, bh
        call putc                      ; seq hi
        mov  di, cx
        mov  al, cl
        call putc                      ; len lo
        mov  al, ch
        call putc                      ; len hi
        test di, di
        jz   .bump
.pay:
        mov  al, [si]
        call putc
        inc  si
        dec  di
        jnz  .pay
.bump:
        test bp, bp
        jz   .done
        inc  word [cs:bp]              ; bump tracked counter after emission
.done:
        pop  es
        pop  ds
        pop  di
        pop  si
        pop  dx
        pop  cx
        pop  bp
        pop  bx
        pop  ax
        ret

; ─────────────────────────── INT-21 hook ───────────────────────────
; Pre/post chaining with a synthesized return frame. The original handler
; ends in IRET, so we hand it a frame (FLAGS, our CS, .after) to return
; through; .after then IRETs on the real caller frame sitting beneath.
int21:
        pushf
        pusha
        push ds
        push es
        ; ---- pre (Task 3b/4): snapshot AH-regs, emit FILE/END ----
        jmp  .chain
.chain:
        pop  es
        pop  ds
        popa
        popf
        pushf                          ; caller FLAGS
        push cs                        ; our CS so the handler IRETs to .after
        push word .after
        jmp  far [cs:old_int21]
.after:
        ; Entered by the original handler's IRET, so the stack holds only the
        ; real caller frame: [caller IP][caller CS][caller FLAGS]. The pushes
        ; below are removed again before iret, so it returns on that frame.
        ; FLAGS/registers here are the original handler's results.
        pusha
        push ds
        push es
        pushf
        pop  word [cs:g_flags]         ; save handler-result FLAGS (caller frame
                                       ; FLAGS sits at [sp+24] once 3 pushes added)
        ; ---- post (Task 3b/4): FILE record, patch pass ----
        pop  es
        pop  ds
        popa
        iret

; ─────────────── INT-60 / INT-1 passthrough (Task 4 fills in) ───────────────
int60:
        jmp  far [cs:old_int60]
int1:
        jmp  far [cs:old_int1]
tsr_end:

; ─────────────────────────── transient install ───────────────────────────
install:
        push cs
        pop  ds

        ; probe COM1: THRE (LSR bit5) must set within a bounded spin
        mov  cx, 0xFFFF
.probe:
        mov  dx, COM1_LSR
        in   al, dx
        test al, 0x20
        jnz  .serial_ok
        loop .probe
        mov  dx, msg_noserial
        call print
        mov  ax, 0x4C01                ; exit AL=1, do NOT go resident
        int  0x21

.serial_ok:
        ; save original vectors
        mov  ax, 0x3521
        int  0x21
        mov  [cs:old_int21], bx
        mov  [cs:old_int21+2], es

        mov  ax, 0x3560
        int  0x21
        mov  [cs:old_int60], bx
        mov  [cs:old_int60+2], es

        mov  ax, 0x3501
        int  0x21
        mov  [cs:old_int1], bx
        mov  [cs:old_int1+2], es

        ; install our vectors
        mov  ax, 0x2521
        mov  dx, int21
        int  0x21

        mov  ax, 0x2560
        mov  dx, int60
        int  0x21

        mov  ax, 0x2501
        mov  dx, int1
        int  0x21

        ; HEADER frame
        push cs
        pop  ds
        mov  al, T_HEADER
        mov  si, hdr_payload
        mov  cx, 6
        call send_frame

        ; stay resident: DX = paragraphs from PSP through tsr_end
        xor  ax, ax
        mov  dx, (tsr_end - start + 0x100 + 15) >> 4
        int  0x27

; print — DX = '$'-terminated string via INT 21h AH=09h. Preserves AX.
print:
        push ax
        mov  ah, 0x09
        int  0x21
        pop  ax
        ret

hdr_payload:  db 'FCAP', 1, 5
msg_noserial: db 'FIFACAP: COM1 not ready, not resident$'

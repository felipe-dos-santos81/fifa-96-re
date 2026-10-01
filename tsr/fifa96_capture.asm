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
pre_ah     db 0                    ; pre-state snapshot
pre_bx     dw 0
pre_cx     dw 0
pre_ds     dw 0
pre_dx     dw 0
pre_name   times 13 db 0           ; AH=3Dh filename copy (<=13 B, NUL-stopped)
pre_name_len db 0
post_ax    dw 0                    ; AX after the original handler ran
post_n     dw 0                    ; AH=3Fh: min(pre_cx,ax_after) valid bytes
post_dlen  dw 0                    ; AH=3Fh: min(post_n,64) bytes dumped
g_flags    dw 0                    ; handler result flags, captured at .after

; ── Task 4a: deferred patch table + per-pass state ──
; Each site record is exactly 13 bytes:
;   dd img_off (4) | db orig_len (1) | db siglen (1) | db sig[4] (4) |
;   dw dump_disp (2) | db dump_len (1)
; sig[0..siglen-1] is the relocation-independent prefix of the live bytes.
; Near-CALL sites use siglen 4 (their 4th byte is the next instruction
; opcode). Site 2 is a far CALL whose operand segment word is relocated by
; the DOS loader at load time (Ghidra image: 9a 12 0b 00 10 = CALLF
; 1000:0b12; live byte 3 differs from 0x00), so only its first 3 bytes are
; compared.
NSITES     equ 5
site_tab:
        dd 0x00007671
        db 5, 4, 0x83,0xbe,0xf6,0xfe
        dw 0xfef6
        db 2
        dd 0x00007678
        db 3, 4, 0xe8,0x27,0x03,0xeb
        dw 0xfef6
        db 2
        dd 0x00007b3e
        db 5, 3, 0x9a,0x12,0x0b,0x00
        dw 0xffe6
        db 2
        dd 0x00007b59
        db 3, 4, 0xe8,0x76,0x01,0x83
        dw 0xfff8
        db 8
        dd 0x00007b1b
        db 3, 4, 0xe8,0xb9,0x09,0x83
        dw 0xfffe
        db 2
site_tab_end:
%if (site_tab_end - site_tab) != NSITES*13
%error "site_tab record stride must be 13 bytes"
%endif
tgt_lin    times NSITES dd 0        ; linear target of each patched site
save0      times NSITES db 0        ; saved live byte 0 of each patched site
save1      times NSITES db 0        ; saved live byte 1 of each patched site
patched    times NSITES db 0        ; processed-this-pass guard
patch_base dd 0                    ; child load base, (PSP+16)<<4

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

; ───────────────────── FILE record build helpers ─────────────────────
; Frozen FILE payload (len = 18+dlen):
;   ah:u8 bx:u16 cx:u16 ds:u16 dx:u16 ax_after:u16 flags:u8 hash:u32
;   dlen:u16 data[dlen]
; flags bit0 hash_valid, bit1 data_is_name, bit2 data_is_bytes.

; put_common — copy the 11-byte prefix (ah,bx,cx,ds,dx,ax_after) into rec_buf.
; Clobbers AX only.
put_common:
        mov  al, [cs:pre_ah]
        mov  [cs:rec_buf], al
        mov  ax, [cs:pre_bx]
        mov  [cs:rec_buf+1], ax
        mov  ax, [cs:pre_cx]
        mov  [cs:rec_buf+3], ax
        mov  ax, [cs:pre_ds]
        mov  [cs:rec_buf+5], ax
        mov  ax, [cs:pre_dx]
        mov  [cs:rec_buf+7], ax
        mov  ax, [cs:post_ax]
        mov  [cs:rec_buf+9], ax
        ret

; rec_zero_extra — clear hash (rec_buf+12..15) and dlen (rec_buf+16..17).
rec_zero_extra:
        mov  word [cs:rec_buf+12], 0
        mov  word [cs:rec_buf+14], 0
        mov  word [cs:rec_buf+16], 0
        ret

; fnv1a — FNV-1a 32 over CX bytes at DS:SI; result hash returned in EAX.
; Clobbers EAX, EBX, CX, SI; leaves DS unchanged. Shared by the FILE read-head
; hash and the INT-60 per-site dump hash.
fnv1a:
        mov  eax, 2166136261           ; FNV-1a basis
        test cx, cx
        jz   .done
.loop:
        movzx ebx, byte [si]
        xor  eax, ebx
        imul eax, eax, 0x01000193      ; * FNV prime 16777619
        inc  si
        dec  cx
        jnz  .loop
.done:
        ret

; fnv_hash — FILE read head: FNV-1a 32 over post_n bytes at pre_ds:pre_dx;
; result u32 LE to rec_buf+12. Preserves all registers (reads via DS).
fnv_hash:
        push eax
        push bx
        push cx
        push si
        push ds
        mov  ax, [cs:pre_ds]
        mov  ds, ax
        mov  si, [cs:pre_dx]
        mov  cx, [cs:post_n]
        call fnv1a
        mov  [cs:rec_buf+12], eax
        pop  ds
        pop  si
        pop  cx
        pop  bx
        pop  eax
        ret

; copy_input — copy post_dlen bytes at pre_ds:pre_dx to cs:rec_buf+18.
; Preserves all registers.
copy_input:
        push ax
        push cx
        push si
        push di
        push ds
        push es
        mov  ax, [cs:pre_ds]
        mov  ds, ax
        mov  si, [cs:pre_dx]
        push cs
        pop  es
        mov  di, rec_buf+18
        mov  cx, [cs:post_dlen]
        cld
        rep  movsb
        pop  es
        pop  ds
        pop  di
        pop  si
        pop  cx
        pop  ax
        ret

; emit_file — CX = payload length; rec_buf filled. Sends T_FILE, bumps the
; 32-bit filecount, and emits a HEARTBEAT (payload = filecount u32) on every
; 1024th FILE record. Preserves AX,BX,CX,DX,SI,DI,BP,DS,ES.
emit_file:
        push ax
        push cx
        push si
        push ds
        push cs
        pop  ds
        mov  al, T_FILE
        mov  si, rec_buf
        call send_frame
        inc  word [cs:filecount]
        jnz  .hb
        inc  word [cs:filecount+2]
.hb:
        test word [cs:filecount], 1023
        jnz  .done
        mov  al, T_HB
        mov  si, filecount
        mov  cx, 4
        call send_frame
.done:
        pop  ds
        pop  si
        pop  cx
        pop  ax
        ret

; ───────────────────── deferred patch pass (Task 4a) ─────────────────────
; do_patch_pass — invoked once on the first INT-21 entry after an AH=4Bh
; launch (guarded by patchpend). Obtains the current (child) PSP from the
; ORIGINAL vector via AH=51h (no IVT recursion), computes the load base
; (PSP+16)<<4, then for each site: range-check target+4 <= 0x100000,
; 4-byte signature match, already-CD60 check; on match saves the 2 live
; bytes, records the linear target, writes CD 60, emits PATCH_OK(0x07);
; every other outcome emits PATCH_SKIP(0x05) with a reason. The only
; writes to game memory are word [target]=CD60 for a signature-matched
; site. Preserves all registers, DS and ES; no INT-21 recursion.
do_patch_pass:
        pushad
        push ds
        push es

        ; --- current PSP via the original vector: AH=51h ---
        pushf
        push cs
        push word .psp_ret
        mov  ah, 0x51
        jmp  far [cs:old_int21]
.psp_ret:
        mov  ax, bx                    ; AX = PSP paragraph
        add  ax, 16
        movzx eax, ax
        shl  eax, 4                    ; EAX = (PSP+16)<<4
        mov  [cs:patch_base], eax

        ; --- clear per-site pass state ---
        xor  di, di
        mov  cx, NSITES
.clr:
        mov  byte [cs:save0+di], 0
        mov  byte [cs:save1+di], 0
        mov  byte [cs:patched+di], 0
        mov  bx, di
        shl  bx, 2
        mov  dword [cs:tgt_lin+bx], 0
        inc  di
        loop .clr

        ; --- walk the site table ---
        xor  di, di
.sitel:
        imul si, di, 13
        add  si, site_tab
        cmp  byte [cs:patched+di], 0
        jne  .next
        mov  byte [cs:patched+di], 1

        mov  eax, [cs:patch_base]
        add  eax, [cs:si]              ; EAX = target linear
        mov  ebx, eax
        add  ebx, 4
        cmp  ebx, 0x100000
        ja   .skip_range

        mov  ebx, eax
        shr  ebx, 4
        mov  ds, bx                    ; DS = target >> 4
        mov  edx, eax
        and  edx, 0x0F
        mov  bx, dx                    ; BX = target & 0xF
        cmp  word [bx], 0x60CD         ; already patched? (checked before sig,
        je   .skip_dup                 ; so re-runs report reason 1, not 0)
        mov  ecx, [bx]                 ; live bytes; CL/CH = bytes 0/1
        mov  dl, [cs:si+5]             ; siglen (1..4)
        mov  dh, [cs:si+6]             ; sig[0]
        cmp  cl, dh
        jne  .skip_sig
        cmp  dl, 1
        je   .sig_ok
        mov  dh, [cs:si+7]             ; sig[1]
        cmp  ch, dh
        jne  .skip_sig
        cmp  dl, 2
        je   .sig_ok
        mov  dh, [cs:si+8]             ; sig[2]
        cmp  byte [bx+2], dh
        jne  .skip_sig
        cmp  dl, 3
        je   .sig_ok
        mov  dh, [cs:si+9]             ; sig[3]
        cmp  byte [bx+3], dh
        jne  .skip_sig

.sig_ok:
        ; --- signature matched: patch ---
        mov  [cs:save0+di], cl
        mov  [cs:save1+di], ch
        mov  word [bx], 0x60CD         ; the one write to game memory
        mov  bx, di
        shl  bx, 2
        mov  [cs:tgt_lin+bx], eax
        mov  [cs:rec_buf], di
        mov  [cs:rec_buf+1], eax
        mov  [cs:rec_buf+5], dl        ; siglen actually matched
        push cs
        pop  ds
        mov  al, T_POK
        mov  si, rec_buf
        mov  cx, 6
        call send_frame
        jmp  .next

.skip_range:
        mov  al, 2
        jmp  .emit_skip
.skip_sig:
        mov  al, 0
        jmp  .emit_skip
.skip_dup:
        mov  al, 1
.emit_skip:
        mov  [cs:rec_buf], di
        mov  [cs:rec_buf+1], al
        push cs
        pop  ds
        mov  al, T_SKIP
        mov  si, rec_buf
        mov  cx, 2
        call send_frame
.next:
        inc  di
        cmp  di, NSITES
        jb   .sitel

        pop  es
        pop  ds
        popad
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
        ; ---- pre: snapshot caller state; END on terminate ----
        mov  [cs:pre_ah], ah
        mov  [cs:pre_bx], bx
        mov  [cs:pre_cx], cx
        mov  [cs:pre_ds], ds
        mov  [cs:pre_dx], dx
        ; deferred patch pass: first INT-21 entry after an AH=4Bh launch
        cmp  byte [cs:patchpend], 1
        jne  .no_patch
        call do_patch_pass
        mov  byte [cs:patchpend], 0
.no_patch:
        cmp  ah, 3Dh
        je   .fname
        cmp  ah, 3Eh
        je   .chain
        cmp  ah, 3Fh
        je   .chain
        cmp  ah, 40h
        je   .chain
        cmp  ah, 4Ch
        je   .quit
        cmp  ah, 4Bh
        je   .patchset
        jmp  .chain
.patchset:                             ; AH=4Bh — arm the deferred patch pass
        mov  byte [cs:patchpend], 1
        jmp  .chain
.fname:                               ; AH=3Dh — copy <=13 B from DS:DX
        mov  si, dx
        mov  di, pre_name
        xor  cx, cx
.fn_loop:
        mov  al, [ds:si]
        mov  [cs:di], al
        test al, al
        jz   .fn_done
        inc  si
        inc  di
        inc  cx
        cmp  cx, 13
        jb   .fn_loop
.fn_done:
        mov  [cs:pre_name_len], cl
        jmp  .chain
.quit:                                ; AH=4Ch — emit END, then chain
        push ds
        push cs
        pop  ds
        mov  al, T_END
        mov  si, rec_buf
        xor  cx, cx
        call send_frame
        pop  ds
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
                                       ; FLAGS sits at [bp+24] once 3 pushes added)
        ; ---- post: read result, dispatch, forward flags ----
        mov  bp, sp                    ; frame base; survives every call below
        mov  di, [bp+18]               ; AX after the original handler
        mov  [cs:post_ax], di
        mov  al, [cs:pre_ah]
        cmp  al, 3Dh
        je   .post_open
        cmp  al, 3Eh
        je   .post_simple
        cmp  al, 40h
        je   .post_simple
        cmp  al, 3Fh
        je   .post_read
        jmp  .post_done
.post_open:                            ; AH=3Dh — name only on success
        call put_common
        test byte [cs:g_flags], 1      ; CF=1 -> failed open
        jnz  .post_open_err
        mov  byte [cs:rec_buf+11], 0x02
        call rec_zero_extra
        mov  al, [cs:pre_name_len]
        mov  [cs:rec_buf+16], al
        mov  byte [cs:rec_buf+17], 0
        mov  si, pre_name
        mov  di, rec_buf+18
        mov  cl, [cs:pre_name_len]
        xor  ch, ch
        jcxz .open_copied
.open_loop:
        mov  al, [cs:si]
        mov  [cs:di], al
        inc  si
        inc  di
        loop .open_loop
.open_copied:
        mov  cl, [cs:pre_name_len]
        xor  ch, ch
        add  cx, 18
        call emit_file
        jmp  .post_done
.post_open_err:                        ; AH=3Dh, CF=1 — no name, no hash
        mov  byte [cs:rec_buf+11], 0
        call rec_zero_extra
        mov  cx, 18
        call emit_file
        jmp  .post_done
.post_simple:                          ; AH=3Eh / 40h — no data
        call put_common
        mov  byte [cs:rec_buf+11], 0
        call rec_zero_extra
        mov  cx, 18
        call emit_file
        jmp  .post_done
.post_read:                            ; AH=3Fh
        test byte [cs:g_flags], 1      ; CF=1 -> failed read
        jnz  .post_read_err
        mov  ax, [cs:pre_cx]
        mov  bx, [cs:post_ax]
        cmp  ax, bx
        jbe  .read_n
        mov  ax, bx
.read_n:
        mov  [cs:post_n], ax
        call fnv_hash
        mov  ax, [cs:post_n]
        cmp  ax, 64
        jbe  .read_dlen
        mov  ax, 64
.read_dlen:
        mov  [cs:post_dlen], ax
        call copy_input
        call put_common
        mov  byte [cs:rec_buf+11], 0x05
        mov  ax, [cs:post_dlen]
        mov  [cs:rec_buf+16], ax
        add  ax, 18
        mov  cx, ax
        call emit_file
        jmp  .post_done
.post_read_err:                        ; AH=3Fh, CF=1
        call put_common
        mov  byte [cs:rec_buf+11], 0
        call rec_zero_extra
        mov  cx, 18
        call emit_file
        jmp  .post_done
.post_done:
        ; forward the handler's result FLAGS to the caller frame's FLAGS slot
        mov  ax, [cs:g_flags]          ; clobbers live AX; popa restores AX below
        mov  [bp+24], ax
        pop  es
        pop  ds
        popa
        iret

; ─────────── INT-60 codec record + INT-1 re-arm (Task 4b) ───────────
; A patched site holds `CD 60`. On entry the INT frame is
;   [sp+0]=IP (site_off+2), [sp+2]=CS, [sp+4]=FLAGS.
; We match linear=(CS<<4)+(IP-2) against tgt_lin[0..4]; a miss chains to the
; original INT-60. On a hit we emit a CODEC record, restore the 2 saved live
; bytes, remember the site in `pending`, and single-step the restored original
; instruction by re-pointing the IRET IP to the site offset with TF set.
; The CPU then raises INT-1 (TF saved, IRET would restore it) after exactly
; that one instruction; int1 rewrites `CD 60` and clears TF in its frame.
;
; Frame base: `push bp / mov bp,sp`, so the INT frame is IP@[bp+2], CS@[bp+4],
; FLAGS@[bp+6]. pusha/push ds/push es then hold the live registers at
; DI@[bp-16] SI@[bp-14] BP@[bp-12] SP@[bp-10] BX@[bp-8] DX@[bp-6] CX@[bp-4]
; AX@[bp-2] DS@[bp-18] ES@[bp-20]; the caller's true BP is [bp]. Because the
; handler returns via iret (not a far jmp) the interrupted site instruction
; sees every register and sp_site = (entry sp)+6 = bp+8.
int60:
        push bp
        mov  bp, sp
        pusha
        push ds
        push es
        ; identify the site: EAX = (frame CS<<4) + (frame IP - 2)
        movzx eax, word [bp+4]
        shl  eax, 4
        movzx ecx, word [bp+2]
        sub  ecx, 2
        add  eax, ecx
        xor  di, di
.site:
        mov  bx, di
        shl  bx, 2
        cmp  eax, [cs:tgt_lin+bx]
        je   .found
        inc  di
        cmp  di, NSITES
        jb   .site
        jmp  .passthru

.found:
        ; ---- CODEC payload: site_id | 13 regs | flags | hash |
        ;                    dseg/doff/dlen | data   (len = 39 + dlen)
        mov  ax, di
        mov  [cs:rec_buf], al
        mov  ax, [bp-2]
        mov  [cs:rec_buf+1], ax        ; ax
        mov  ax, [bp-8]
        mov  [cs:rec_buf+3], ax        ; bx
        mov  ax, [bp-4]
        mov  [cs:rec_buf+5], ax        ; cx
        mov  ax, [bp-6]
        mov  [cs:rec_buf+7], ax        ; dx
        mov  ax, [bp-14]
        mov  [cs:rec_buf+9], ax        ; si
        mov  ax, [bp-16]
        mov  [cs:rec_buf+11], ax       ; di
        mov  ax, [bp]
        mov  [cs:rec_buf+13], ax       ; bp (caller)
        lea  ax, [bp+8]
        mov  [cs:rec_buf+15], ax       ; sp_site
        mov  ax, [bp-18]
        mov  [cs:rec_buf+17], ax       ; ds
        mov  ax, [bp-20]
        mov  [cs:rec_buf+19], ax       ; es
        mov  ax, ss
        mov  [cs:rec_buf+21], ax       ; ss
        mov  ax, [bp+2]
        sub  ax, 2
        mov  [cs:rec_buf+23], ax       ; ip_site
        mov  ax, [bp+4]
        mov  [cs:rec_buf+25], ax       ; cs_site
        mov  ax, [bp+6]
        mov  [cs:rec_buf+27], ax       ; flags_site
        ; ---- per-site dump: SS:[BP+disp], dlen from the table (BP/SS caller's)
        imul si, di, 13
        add  si, site_tab
        mov  bx, [cs:si+10]            ; dump_disp
        mov  ax, [bp]                  ; caller BP
        add  ax, bx
        mov  [cs:rec_buf+35], ax       ; doff = BP + disp
        mov  cl, [cs:si+12]            ; dump_len
        xor  ch, ch
        mov  [cs:rec_buf+37], cx       ; dlen
        mov  ax, ss
        mov  [cs:rec_buf+33], ax       ; dseg = SS
        mov  ds, ax
        mov  dx, [bp]
        add  dx, bx                    ; DX = window offset (survives fnv1a)
        mov  si, dx
        call fnv1a                     ; EAX = hash (clobbers EBX,CX,SI)
        mov  [cs:rec_buf+29], eax
        push cs
        pop  es
        cld
        mov  si, dx
        mov  di, rec_buf+39
        mov  cx, [cs:rec_buf+37]
        rep  movsb                     ; DS:SI (SS:BP+disp) -> ES:DI (rec_buf)
        ; ---- emit CODEC (type 0x03) with DS=CS
        push cs
        pop  ds
        mov  al, T_CODEC
        mov  si, rec_buf
        mov  cx, [cs:rec_buf+37]
        add  cx, 39
        call send_frame
        ; ---- un-patch: put the 2 saved live bytes back at the site
        movzx di, byte [cs:rec_buf]
        mov  bx, di
        shl  bx, 2
        mov  eax, [cs:tgt_lin+bx]      ; EAX = target linear
        mov  edx, eax
        and  edx, 0x0F                 ; EDX = target & 0xF (linear->offset)
        shr  eax, 4
        mov  ds, ax                    ; DS = target >> 4
        mov  si, [bp+2]
        sub  si, 2                     ; SI = site's original offset (frame IP-2)
        mov  bx, dx
        mov  dl, [cs:save0+di]
        mov  dh, [cs:save1+di]
        mov  [bx], dx                  ; original 2 bytes restored
        ; ---- remember the site, then re-point the IRET frame.
        ; NB: the site's original offset is frame_IP-2 (e.g. map 11bd:5aa1 ->
        ; frame IP-2 = 0x5aa1), NOT tgt_lin & 0xF: the code segment at the site
        ; is the relocated map segment, not the linear's canonical paragraph,
        ; so CS is left as-is and IP must be the instruction's real offset.
        mov  ax, di
        mov  [cs:pending], al
        mov  [bp+2], si                ; IP = site offset
        or   word [bp+6], 0x0100       ; FLAGS |= TF
        pop  es
        pop  ds
        popa
        pop  bp
        iret

.passthru:
        pop  es
        pop  ds
        popa
        pop  bp
        jmp  far [cs:old_int60]

; INT-1 re-arm: if a site hit is pending, rewrite `CD 60` there, clear the
; pending flag, clear TF in the INT-1 frame (the CPU saves TF as it was set,
; so a plain iret would resume single-stepping) and iret. Otherwise chain.
int1:
        push bp
        mov  bp, sp
        cmp  byte [cs:pending], 0FFh
        jne  .consume
        pop  bp
        jmp  far [cs:old_int1]
.consume:
        push ax
        push bx
        push dx
        push ds
        movzx bx, byte [cs:pending]
        shl  bx, 2
        mov  eax, [cs:tgt_lin+bx]      ; EAX = target linear
        mov  edx, eax
        shr  eax, 4
        mov  ds, ax                    ; DS = target segment
        and  dx, 0x0F
        mov  bx, dx
        mov  word [bx], 0x60CD         ; re-arm the site
        mov  byte [cs:pending], 0FFh
        pop  ds
        pop  dx
        pop  bx
        pop  ax
        and  word [bp+6], 0xFEFF       ; clear TF in the INT-1 frame
        pop  bp
        iret
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

        ; stay resident: DX = paragraphs from PSP through tsr_end.
        ; AH=31h, not INT 27h: DOSBox-X's DOS_27Handler computes its
        ; paragraph count as reg_dx/16 (it treats DX as bytes), so an INT 27h
        ; TSR keeps 1/16 of the memory it asked for and the next program's PSP
        ; overwrites the resident data (observed: saved old_int21 clobbered,
        ; first chained INT-21 call hangs). AH=31h resizes by DX paragraphs.
        mov  ax, 0x3100
        mov  dx, (tsr_end - start + 0x100 + 15) >> 4
        int  0x21

; print — DX = '$'-terminated string via INT 21h AH=09h. Preserves AX.
print:
        push ax
        mov  ah, 0x09
        int  0x21
        pop  ax
        ret

hdr_payload:  db 'FCAP', 1, 5
msg_noserial: db 'FIFACAP: COM1 not ready, not resident$'

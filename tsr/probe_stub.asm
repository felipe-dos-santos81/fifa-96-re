; tsr/probe_stub.asm — INT 61h smoke: sets four sentinel values and exits.
; nasm -f bin tsr/probe_stub.asm -o STUB.COM
        org 0x100
        bits 16
        mov  esi, 0x0000BEEF
        mov  ebx, 0x00001234
        mov  edx, 0x00005678
        mov  ebp, 0x00009ABC
        int  0x61
        mov  ax, 0x4C00
        int  0x21

;
; LONGMATH.ASM -- 32-bit math runtime helpers for Small-C long int support.
;
; Register convention:
;   DX:AX = primary  (d1, left operand)
;   CX:BX = secondary (d2, right operand)
;   Result in DX:AX  (or AX=0/1, DX=0 for comparisons)
;
; Uses DB 0C3H for RET to match CALL.ASM convention.
;
; *** SMALL-C CALLING CONVENTION (left-to-right push) ***
; Small-C pushes arguments LEFT-TO-RIGHT (first arg pushed first).
; This is the OPPOSITE of standard C / cdecl (which pushes right-to-left).
; For a function f(p1, p2, p3), the stack on entry looks like:
;
;   [BP+4]  = p3  (last arg,   pushed last  -> lowest address)
;   [BP+6]  = p2  (middle arg)
;   [BP+8]  = p1  (first arg,  pushed first -> highest address)
;
; General: arg N (1-based of nargs total) is at [BP + 2*(nargs - N + 2)]
;
; For a 2-arg function the layout is symmetrical and matches cdecl.
;

CODE    SEGMENT PUBLIC
        ASSUME  CS:CODE

;======================================================================
; Logical negate:  DX:AX = !(DX:AX)  =>  AX=0/1, DX=0
;======================================================================
        PUBLIC __llneg
__llneg:
        OR   AX,DX
        JNZ  LNNO
        MOV  AX,1
        DB   0C3H
LNNO:   XOR  AX,AX
        XOR  DX,DX
        DB   0C3H

;======================================================================
; Multiply:  DX:AX = DX:AX * CX:BX  (low 32 bits only)
; Signed and unsigned produce the same low 32 bits.
;======================================================================
        PUBLIC __lmul
        PUBLIC __ulmul
__lmul:
__ulmul:
        PUSH AX
        MOV  AX,DX
        MUL  BX
        XCHG CX,AX
        POP  DX
        PUSH DX
        XCHG AX,DX
        MUL  DX
        ADD  CX,AX
        POP  AX
        MUL  BX
        ADD  DX,CX
        DB   0C3H

;======================================================================
; Signed divide:  DX:AX = DX:AX / CX:BX  (quotient)
;======================================================================
        PUBLIC __ldiv
__ldiv:
        PUSH SI
        PUSH DI
        PUSH BP
        MOV  SI,DX
        XOR  SI,CX
        TEST DX,DX
        JNS  DV1
        NEG  DX
        NEG  AX
        SBB  DX,0
DV1:    TEST CX,CX
        JNS  DV2
        NEG  CX
        NEG  BX
        SBB  CX,0
DV2:    PUSH SI
        MOV  SI,AX
        MOV  DI,DX
        XOR  AX,AX
        XOR  DX,DX
        MOV  BP,32
DVLP:   SHL  SI,1
        RCL  DI,1
        RCL  AX,1
        RCL  DX,1
        CMP  DX,CX
        JA   DVSB
        JB   DVNX
        CMP  AX,BX
        JB   DVNX
DVSB:   SUB  AX,BX
        SBB  DX,CX
        INC  SI
DVNX:   DEC  BP
        JNZ  DVLP
        MOV  AX,SI
        MOV  DX,DI
        POP  SI
        TEST SI,SI
        JNS  DVDN
        NEG  DX
        NEG  AX
        SBB  DX,0
DVDN:   POP  BP
        POP  DI
        POP  SI
        DB   0C3H

;======================================================================
; Unsigned divide:  DX:AX = DX:AX / CX:BX  (quotient)
;======================================================================
        PUBLIC __uldiv
__uldiv:
        PUSH SI
        PUSH DI
        PUSH BP
        MOV  SI,AX
        MOV  DI,DX
        XOR  AX,AX
        XOR  DX,DX
        MOV  BP,32
UDLP:   SHL  SI,1
        RCL  DI,1
        RCL  AX,1
        RCL  DX,1
        CMP  DX,CX
        JA   UDSB
        JB   UDNX
        CMP  AX,BX
        JB   UDNX
UDSB:   SUB  AX,BX
        SBB  DX,CX
        INC  SI
UDNX:   DEC  BP
        JNZ  UDLP
        MOV  AX,SI
        MOV  DX,DI
        POP  BP
        POP  DI
        POP  SI
        DB   0C3H

;======================================================================
; Signed modulo:  DX:AX = DX:AX % CX:BX  (remainder, sign=dividend)
;======================================================================
        PUBLIC __lmod
__lmod:
        PUSH SI
        PUSH DI
        PUSH BP
        MOV  SI,DX
        TEST DX,DX
        JNS  MD1
        NEG  DX
        NEG  AX
        SBB  DX,0
MD1:    TEST CX,CX
        JNS  MD2
        NEG  CX
        NEG  BX
        SBB  CX,0
MD2:    PUSH SI
        MOV  SI,AX
        MOV  DI,DX
        XOR  AX,AX
        XOR  DX,DX
        MOV  BP,32
MDLP:   SHL  SI,1
        RCL  DI,1
        RCL  AX,1
        RCL  DX,1
        CMP  DX,CX
        JA   MDSB
        JB   MDNX
        CMP  AX,BX
        JB   MDNX
MDSB:   SUB  AX,BX
        SBB  DX,CX
        INC  SI
MDNX:   DEC  BP
        JNZ  MDLP
        POP  SI
        TEST SI,SI
        JNS  MDDN
        NEG  DX
        NEG  AX
        SBB  DX,0
MDDN:   POP  BP
        POP  DI
        POP  SI
        DB   0C3H

;======================================================================
; Unsigned modulo:  DX:AX = DX:AX % CX:BX  (remainder)
;======================================================================
        PUBLIC __ulmod
__ulmod:
        PUSH SI
        PUSH DI
        PUSH BP
        MOV  SI,AX
        MOV  DI,DX
        XOR  AX,AX
        XOR  DX,DX
        MOV  BP,32
UMLP:   SHL  SI,1
        RCL  DI,1
        RCL  AX,1
        RCL  DX,1
        CMP  DX,CX
        JA   UMSB
        JB   UMNX
        CMP  AX,BX
        JB   UMNX
UMSB:   SUB  AX,BX
        SBB  DX,CX
        INC  SI
UMNX:   DEC  BP
        JNZ  UMLP
        POP  BP
        POP  DI
        POP  SI
        DB   0C3H

;======================================================================
; Shift left:  CX:BX << AX  =>  result in DX:AX
; At entry: DX:AX=count (primary), CX:BX=value (secondary)
; Need: value in DX:AX, count in CX
;======================================================================
        PUBLIC __lshl
__lshl:
        XCHG CX,DX          ; CX=count_hi(discard), DX=value_hi
        XCHG AX,BX          ; AX=value_lo, BX=count_lo(discard)
        MOV  CX,BX          ; CX=count_lo
        AND  CX,31
        JCXZ SLDN
SLLP:   SHL  AX,1
        RCL  DX,1
        LOOP SLLP
SLDN:   
        DB   0C3H

;======================================================================
; Arithmetic shift right (signed):  CX:BX >> AX  =>  result in DX:AX
; At entry: DX:AX=count (primary), CX:BX=value (secondary)
;======================================================================
        PUBLIC __lsar
__lsar:
        XCHG CX,DX
        XCHG AX,BX
        MOV  CX,BX
        AND  CX,31
        JCXZ ARDN
ARLP:   SAR  DX,1
        RCR  AX,1
        LOOP ARLP
ARDN:   
        DB   0C3H

;======================================================================
; Logical shift right (unsigned):  CX:BX >> AX  =>  result in DX:AX
; At entry: DX:AX=count (primary), CX:BX=value (secondary)
;======================================================================
        PUBLIC __lshr
__lshr:
        XCHG CX,DX
        XCHG AX,BX
        MOV  CX,BX
        AND  CX,31
        JCXZ SRDN
SRLP:   SHR  DX,1
        RCR  AX,1
        LOOP SRLP
SRDN:   
        DB   0C3H

;======================================================================
; Comparisons:  CX:BX <op> DX:AX  =>  AX=0/1, DX=0
; Uses shared YES/NO labels (like CALL.ASM's true/false pattern).
;======================================================================

; --- Equal ---
        PUBLIC __leq
__leq:
        CMP  CX,DX
        JNE  NO32
        CMP  BX,AX
        JNE  NO32
YES32:  MOV  AX,1
        XOR  DX,DX
        DB   0C3H
NO32:   XOR  AX,AX
        XOR  DX,DX
        DB   0C3H

; --- Not Equal ---
        PUBLIC __lne
__lne:
        CMP  CX,DX
        JNE  YES32
        CMP  BX,AX
        JNE  YES32
        JMP  NO32

; --- Signed Less Than:  CX:BX < DX:AX ---
        PUBLIC __llt
__llt:
        CMP  CX,DX
        JL   YES32
        JG   NO32
        CMP  BX,AX
        JB   YES32
        JMP  NO32

; --- Signed Less or Equal:  CX:BX <= DX:AX ---
        PUBLIC __lle
__lle:
        CMP  CX,DX
        JL   YES32
        JG   NO32
        CMP  BX,AX
        JBE  YES32
        JMP  NO32

; --- Signed Greater Than:  CX:BX > DX:AX ---
        PUBLIC __lgt
__lgt:
        CMP  CX,DX
        JG   YES32
        JL   NO32
        CMP  BX,AX
        JA   YES32
        JMP  NO32

; --- Signed Greater or Equal:  CX:BX >= DX:AX ---
        PUBLIC __lge
__lge:
        CMP  CX,DX
        JG   YES32
        JL   NO32
        CMP  BX,AX
        JAE  YES32
        JMP  NO32

; --- Unsigned Less Than:  CX:BX < DX:AX ---
        PUBLIC __ullt
__ullt:
        CMP  CX,DX
        JB   YES32
        JA   NO32
        CMP  BX,AX
        JB   YES32
        JMP  NO32

; --- Unsigned Less or Equal:  CX:BX <= DX:AX ---
        PUBLIC __ulle
__ulle:
        CMP  CX,DX
        JB   YES32
        JA   NO32
        CMP  BX,AX
        JBE  YES32
        JMP  NO32

; --- Unsigned Greater Than:  CX:BX > DX:AX ---
        PUBLIC __ulgt
__ulgt:
        CMP  CX,DX
        JA   YES32
        JB   NO32
        CMP  BX,AX
        JA   YES32
        JMP  NO32

; --- Unsigned Greater or Equal:  CX:BX >= DX:AX ---
        PUBLIC __ulge
__ulge:
        CMP  CX,DX
        JA   YES32
        JB   NO32
        CMP  BX,AX
        JAE  YES32
        JMP  NO32

;======================================================================
; 32-bit switch:  DX:AX = switch value
; Table follows CALL (return address -> table in CS):
;   DW case_label, DW lo_val, DW hi_val  (6 bytes per entry)
;   DW 0, DW 0, DW 0                     (terminator)
;  [JMP default]  or continuation
;======================================================================
        PUBLIC __lswitch
__lswitch:
        POP  BX              ; BX -> switch table (return addr)
        JMP  LSKIP
LBACK:  ADD  BX,6
LSKIP:  MOV  CX,CS:[BX]     ; CX = case label
        JCXZ LDEF            ; 0 = end of table
        CMP  DX,CS:[BX+4]   ; compare hi word
        JNE  LBACK
        CMP  AX,CS:[BX+2]   ; compare lo word
        JNE  LBACK
        JMP  CX              ; match: jump to case label
LDEF:   ADD  BX,6            ; skip terminator
        JMP  BX              ; jump to default/continuation

;======================================================================
; udiv3216(plo, phi, divisor)
; Divides unsigned 32-bit value *phi:*plo by 16-bit divisor.
; Updates *plo and *phi with quotient. Returns remainder in AX.
; Stack: [BP+4]=plo, [BP+6]=phi, [BP+8]=divisor
;======================================================================
; Note: Small-C pushes arguments left-to-right, so for udiv3216(plo, phi, divisor):
;   plo  pushed 1st -> [BP+8]
;   phi  pushed 2nd -> [BP+6]
;   divisor pushed 3rd (last) -> [BP+4]
        PUBLIC _udiv3216
_udiv3216:
        PUSH BP
        MOV  BP,SP
        PUSH SI
        MOV  SI,[BP+6]     ; SI = phi     (2nd arg)
        MOV  BX,[BP+8]     ; BX = plo     (1st arg, was wrong: [BP+4])
        MOV  CX,[BP+4]     ; CX = divisor (3rd arg, was wrong: [BP+8])
        ; Step 1: divide hi word by divisor
        XOR  DX,DX
        MOV  AX,[SI]       ; AX = *phi
        DIV  CX             ; AX = hi_quot, DX = hi_rem
        MOV  [SI],AX        ; *phi = hi_quot
        ; Step 2: divide (hi_rem:lo) by divisor
        ; DX already has hi_rem from step 1
        MOV  AX,[BX]       ; AX = *plo
        DIV  CX             ; AX = lo_quot, DX = remainder
        MOV  [BX],AX        ; *plo = lo_quot
        MOV  AX,DX          ; return remainder in AX
        XOR  DX,DX          ; clear DX (return is int, not long)
        POP  SI
        POP  BP
        ; ret (0C3h)
        DB   0C3H

CODE ENDS
        END

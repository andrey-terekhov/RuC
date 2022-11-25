szof:
                                  # a0 - type
    addi  $v0, $0, 2
    addi  $t0, $0, -2             # CHAR
    beq   $a0, $t0, szofEND

    addi  $v0, $0, 4
    addi  $t0, $0, -1             # INT
    beq   $a0, $t0, szofEND

    addi  $t0, $0, -3             # FLOAT
    beq   $a0, $t0, szofEND

    lui   $t0, %hi(modetab)
    addiu $t0, $t0, %lo(modetab)  # t0 modetab
    sll   $t1, $a0, 2             # t1 type*4
    add   $t2, $t1, $t0           # t2 address of modetab[t]
    lw    $t0, 0($t2)             # t0 ARRAY or STRUCT or POINTER

    addi  $t1, $0, 1003           # ARRAY
    beq   $t0, $t1, szofEND

    addi  $t1, $0, 1004           # POINTER
    beq   $t0, $t1, szofEND

    lw    $v0, 4($t2)             # STRUCT
szofEND:
    jr    $ra


szofstr:
                                  # a0 - type
    addi  $v0, $0, 4
    addi  $t0, $0, -2             # CHAR
    beq   $a0, $t0, szofstrEND

    addi  $t0, $0, -1             # INT
    beq   $a0, $t0, szofstrEND

    addi  $t0, $0, -3             # FLOAT
    beq   $a0, $t0, szofstrEND

    lui   $t0, %hi(modetab)
    addiu $t0, $t0, %lo(modetab)  # t0 modetab
    sll   $t1, $a0, 2             # t1 type*4
    add   $t2, $t1, $t0           # t2 address of modetab[t]
    lw    $t0, 0($t2)             # t0 ARRAY or STRUCT or POINTER

    addi  $t1, $0, 1003           # ARRAY
    beq   $t0, $t1, szofstrEND

    addi  $t1, $0, 1004           # POINTER
    beq   $t0, $t1, szofstrEND

    lw    $v0, 4($t2)             # STRUCT
szofstrEND:
    jr    $ra



SLICE4:
	lw $t0, -4($a0)
	bltz $a1, SLICE4ERR
	sub $t1, $a1, $t0
	bltz $t1, SLICE4OK
SLICE4ERR:
    addi $a0, $0, 5   # wrong index
    add $a2, $0, $t0  # N, в a1 index
    jal error
	nop
SLICE4OK:
	sll $t0, $a1, 2
	add $v0, $t0, $a0
	jr $ra
	nop





SLICE2:
	lw $t0, -4($a0)
	bltz $a1, SLICE1ERR
	sub $t1, $a1, $t0
	bltz $t1, SLICE1OK
SLICE1ERR:
    addi $a0, $0, 5   # wrong index
    add $a2, $0, $t0  # N, в a1 index
	jal error
	nop
SLICE1OK:
	add $a1, $a1, $a1
	add $v0, $a1, $a0
	jr $ra
	nop





SLICE:
    lw $t0, -4($a0)           # в a0 адрес нулевого элемента
    bltz $a1, SLICEERR        # в a1 индекс
    sub $t1, $a1, $t0         # в a2 шаг
    bltz $t1, SLICEOK         # в t0 N
SLICEERR:
    addi $a0, $0, 5           # wrong index
    add $a2, $0, $t0          # N, в a1 index
    jal error
	nop
SLICEOK:
	mul $t0, $a1, $a2
    add $v0, $t0, $a0         # в v0 результат- адрес a[i]
	jr $ra
	nop



print_char:            # a0 - это wchar
	addi $fp, $fp, -32
	sw $sp, 20($fp)
	move $sp, $fp
	sw $ra, 16($sp)
    sw $s0, 24($sp)

	addi $t1, $a0, -128
	bgez $t1, prELSE1
	.rdata
	.align 2
prSTRING1:
	.ascii "%c\0"
	.text
	.align 2
	move $a1, $a0
	lui $t1, %hi(prSTRING1)
	addiu $a0, $t1, %lo(prSTRING1)
    move $s0, $sp
    addi $sp, $sp, -16
	jal printf
	nop
    move $sp, $s0
	j prEND2
	nop
prELSE1:
	srl $t0, $a0, 6
    ori $a1, $t0, 0xC0  # first
    andi $t0, $a0, 0x3F
    ori $a2, $t0, 0x80  # second

	.rdata
	.align 2
prSTRING2:
	.ascii "%c%c\0"
	.text
	.align 2
	lui $t1, %hi(prSTRING2)
	addiu $a0, $t1, %lo(prSTRING2)
    move $s0, $sp
    addi $sp, $sp, -16
    jal printf
    nop
    move $sp, $s0
prEND2:
    lw $s0, 24($sp)
	lw $ra, 16($sp)
	addi $fp, $sp, 32
	lw $sp, 20($sp)
	jr $ra
	nop




error:
	addi $fp, $fp, -32
	sw $sp, 20($fp)
	move $sp, $fp
	sw $ra, 16($sp)

    sw   $s0, 24($sp)
	move $s0, $a0
CASE01:
	addi $t0, $0, 1
	bne $t0, $s0, CASE03
	.rdata
	.align 2
	.word 27
STRING01:
	.ascii "1.:>=G8;8AL @538AB@K s0-s7\n\0"
	.text
	.align 2
	lui $t1, %hi(STRING01)
	addiu $a0, $t1, %lo(STRING01)
	jal printf
	nop
	j ENDERR
	nop
CASE03:
	addi $t0, $0, 2
	bne $t0, $s0, CASE04
	.rdata
	.align 2
	.word 29
STRING02:
	.ascii "2.:>=G8;8AL @538AB@K fs0-fs5\n\0"
	.text
	.align 2
	lui $t1, %hi(STRING02)
	addiu $a0, $t1, %lo(STRING02)
	jal printf
	nop
	j ENDERR
	nop
CASE04:
	addi $t0, $0, 3
	bne $t0, $s0, CASE05
	.rdata
	.align 2
	.word 42
STRING03:
	.ascii "3.?>:0 :>;8G5AB2> ?0@0<5B@>2 >3@0=8G5=> 4\n\0"
	.text
	.align 2
	lui $t1, %hi(STRING03)
	addiu $a0, $t1, %lo(STRING03)
	jal printf
	nop
	j ENDERR
	nop
CASE05:
	addi $t0, $0, 4
	bne $t0, $s0, CASE06
	.rdata
	.align 2
	.word 53
STRING04:
	.ascii "4.2  C!8 @07<5@=>ABL <0AA82>2 =5 <>65B 1KBL 1>;LH5 5\n\0"
	.text
	.align 2
	lui $t1, %hi(STRING04)
	addiu $a0, $t1, %lo(STRING04)
	jal printf
	nop
	j ENDERR
	nop
CASE06:
	addi $t0, $0, 5
	bne $t0, $s0, CASE07
	.rdata
	.align 2
	.word 41
STRING05:
	.ascii "5.индекс меньше 0 илит больше N-1\n\0"
	.text
	.align 2
	lui $t1, %hi(STRING05)
	addiu $a0, $t1, %lo(STRING05)
	jal printf
	nop
	j ENDERR
	nop
CASE07:
	addi $t0, $0, 6
	bne $t0, $s0, CASE08
	.rdata
	.align 2
	.word 20
STRING06:
	.ascii "6.8AG5@?0=85 ?0<OB8\n\0"
	.text
	.align 2
	lui $t1, %hi(STRING06)
	addiu $a0, $t1, %lo(STRING06)
	jal printf
	nop
	j ENDERR
	nop
CASE08:
	addi $t0, $0, 7
	bne $t0, $s0, CASE09
	.rdata
	.align 2
	.word 46
STRING07:
	.ascii "7.3@0=8F0 <0AA820 =5 <>65B 1KBL >B@8F0B5;L=>9\n\0"
	.text
	.align 2
	lui $t1, %hi(STRING07)
	addiu $a0, $t1, %lo(STRING07)
	jal printf
	nop
	j ENDERR
	nop
CASE09:

ENDERR:
    lw $s0, 24($sp)
	lw $ra, 16($sp)
	addi $fp, $sp, 32
	lw $sp, 20($sp)
	jr $ra
	nop



auxprint:
    addi $fp, $fp, -64    # a0 arg address
    sw $sp, 20($fp)       # a1 t
    move $sp, $fp         # a2 before
    sw $ra, 16($sp)       # a3 after

    sw $s0, 24($sp)
    sw $s1, 28($sp)
    sw $s2, 32($sp)
    sw $s3, 36($sp)
    sw $s4, 40($sp)
    sw $s5, 44($sp)
    sw $s6, 48($sp)
    sw $s7, 52($sp)

    move $s0, $a0         # s0 arg
    move $s1, $a1         # s1 type
    move $s2, $a3
    beq  $a2, $0, auxprELSE1
	.rdata
	.align 2
auxprSTRING0:
	.ascii "%c\0"
	.text
	.align 2
    move $a1, $a2         # before
	lui $t1, %hi(auxprSTRING0)
	addiu $a0, $t1, %lo(auxprSTRING0)
    move  $s7, $sp
    addi  $sp, $fp, -16
    jal   printf
    nop
    move  $sp, $s7
auxprELSE1:
    addi $t0, $0, -1
    bne $s1, $t0, auxprELSE2
	.rdata
	.align 2
auxprSTRING1:
	.ascii "%i\0"
	.text
	.align 2
    lw  $a1, 0($s0)
	lui $t1, %hi(auxprSTRING1)
	addiu $a0, $t1, %lo(auxprSTRING1)
    move  $s7, $sp
    addi  $sp, $fp, -16
    jal   printf
    nop
    move  $sp, $s7
	j auxprEND
	nop
auxprELSE2:
	addi $t0, $0, -2
	bne $s1, $t0, auxprELSE3
    lhu $a0, 0($s0)
    jal print_char
    nop
auxprELSE3:
	addi $t0, $0, -3
	bne $s1, $t0, auxprELSE4
	.rdata
	.align 2
auxprSTRING3:
	.ascii "%f\0"
	.text
	.align 2
	lwc1 $f4, 0($s0)
	cvt.d.s $f4,$f4
	mfc1	$5,$f4
	mfhc1	$6,$f4
	lui $t1, %hi(auxprSTRING3)
	addiu $a0, $t1, %lo(auxprSTRING3)
    move  $s7, $sp
    addi  $sp, $fp, -16
    jal   printf
    nop
    move  $sp, $s7
auxprELSE4:
    lui $t1, %hi(modetab)
    addiu $t1, $t1, %lo(modetab)  # t1 modetab
    sll   $t0, $s1, 2             # t0 type*4
    add   $s3, $t1, $t0           # s3 address of modetab[t]
    lw    $t0, 0($s3)             # t0 MARRAY or MSTRUCT or MPOINTER
    addi  $t1, $0, 1003           # MARRAY
    bne   $t0, $t1, auxprELSE7

    lw    $s4, 4($s3)             # s4 type of elem
    move  $a0, $s4
    jal   szof
    nop
    move  $s5, $v0                # s5 d

    lw    $s0, 0($s0)
    lw    $t0, -4($s0)             # N
    mul   $t0, $s5, $t0
    add   $s6, $s0, $t0           # s6 address of a[N]

auxprELSE5:
    sub   $t0, $s0, $s6
    bgez  $t0, auxprEND

    move  $a0, $s0                # address a[i]
    move  $a1, $s4                # type of elem
    move  $a2, $0
    addi  $a3, $0, '\n'
    bgez  $s4, auxprELSE6         # type > 0
    move  $a3, $0
    addi  $t0, $0, -2             # LCHAR
    beq   $s4, $t0, auxprELSE6
    addi  $a3, $0, ' '
auxprELSE6:
    jal   auxprint
    nop
    add   $s0, $s0, $s5
    j     auxprELSE5
    nop
     
auxprELSE7:
    addi  $t1, $0, 1002           # MSTRUCT
    bne   $t0, $t1, auxprEND
    lw    $s6, 8($s3)             # cnt
    addi  $s7, $0, 2              # i
    .rdata
    .align 2
auxprSTRING4:
    .ascii "{\0"
    .text
    .align 2
    lui $t1, %hi(auxprSTRING4)
    addiu $a0, $t1, %lo(auxprSTRING4)
    sw    $s7, 56($sp)
    move  $s7, $sp
    addi  $sp, $fp, -16
    jal   printf
    nop
    move  $sp, $s7
    lw    $s7, 56($sp)
auxprLOOP:
    sub  $t0, $s7, $s6            # i <= cnt
    bgtz $t0, auxprELSE12
    sll  $t0, $s7, 2              # i*4
    add  $t1, $s3, $t0
    lw   $s4, 4($t1)               # type = modetab[t+i+1]
    move $a0, $s0
    move $a1, $s4
    move $a2, $0
    bgez $s4, auxprELSE8
    addi $t0, $0, 2               # type < 0
    beq  $s7, $t0, auxprELSE9     # i == 2
    addi $a2, $0, ' '
    j auxprELSE9
    nop
auxprELSE8:
    addi $a2, $0, '\n'            # type > 0
auxprELSE9:
    move $a3, $0
    bgez $s4, auxprELSE10         # type < 0
    beq  $s7, $s6, auxprELSE11    # i == cnt
    addi $a3, $0, ','
    j auxprELSE11
    nop
auxprELSE10:
    addi $a3, $0, '\n'            # type > 0
auxprELSE11:
    jal auxprint
    nop
    move  $a0, $s4
    jal   szofstr
    nop
    add  $s0, $s0, $v0
    addi $s7, $s7, 2
    j auxprLOOP
    nop

    .rdata
    .align 2
auxprSTRING5:
    .ascii "}\0"
    .text
    .align 2
auxprELSE12:
    lui $t1, %hi(auxprSTRING5)
    addiu $a0, $t1, %lo(auxprSTRING5)
    move  $s7, $sp
    addi  $sp, $fp, -16
    jal   printf
    nop
    move  $sp, $s7

auxprEND:
    beq  $s2, $0, auxprELSE13
    .rdata
    .align 2
auxprSTRING6:
    .ascii "%c\0"
    .text
    .align 2
    move $a1, $s2         # after 
    lui $t1, %hi(auxprSTRING6)
    addiu $a0, $t1, %lo(auxprSTRING6)
    move  $s7, $sp
    addi  $sp, $fp, -16
    jal   printf
    nop
    move  $sp, $s7
auxprELSE13:
    lw $s0, 24($sp)
    lw $s1, 28($sp)
    lw $s2, 32($sp)
    lw $s3, 36($sp)
    lw $s4, 40($sp)
    lw $s5, 44($sp)
    lw $s6, 48($sp)
    lw $s7, 52($sp)

	lw $ra, 16($sp)
	addi $fp, $sp, 64
	lw $sp, 20($sp)
	jr $ra
	nop


# объявление одномерного массива
# $a0 -- адрес первого элемента
# $a1 -- размер измерения
DEFARR1:
	sw $a1, 4($a0)			# Сохранение границы
	li $v0, 4				# Загрузка размера слова
	mul $v0, $v0, $a1		# Подсчёт размера первого измерения массива в байтах
	sub $v0, $a0, $v0		# Считаем адрес после конца массива, т.е. $v0 -- на слово ниже последнего элемента
	jr $ra

# объявление многомерного массива, но сначала обязана вызываться процедура DEFARR1
# $a0 -- адрес первого элемента
# $a1 -- размер измерения
# $a2 -- адрес первого элемента предыдущего измерения
# $a3 -- размер предыдущего измерения
DEFARR2:
	sw $a0, 0($a2)			# Сохраняем адрес в элементе предыдущего измерения
	move $t0, $ra			# Запоминаем $ra, чтобы он не затёрся
	jal DEFARR1				# Выделение памяти под массив
	move $ra, $t0			# Восстанавливаем $ra
	addi $a2, $a2, -4		# В $a2 следующий элемент в предыдущем измерении
	addi $a0, $v0, -4		# В $a0 первый элемент массива в текущем измерении
	addi $a3, $a3, -1		# Уменьшаем счётчик
	bne $a3, $0, DEFARR2	# Прыгаем, если ещё не всё выделили
	jr $ra

ARASSNI:                 # присваивание целых массивов произвольной размерности
    # a0 куда (адрес 0-го элемента)
	# a1 откуда (адрес 0-го элемента)
	# a2 размерность Dim
# globinit = -8000, соответственно, leftС0 = -8000 от gp, rightC0 = -8020, ind = -8040 от gp
# DISP0 = 80
    addi $t1, $0, 1      # curdim в t1
    move $t5, $0 # ind = 0
ARASSNI1:
    lw $t3, -4($a0)      # N левого массива
    lw $t4, -4($a1)      # N правого массива
    beq $t3, $t4, ARASSNI2
    addi $a0, $0, 13    # не совпадает количество элементов при присваивании массивов
    jal error
    nop
ARASSNI2:
    sub $t0, $t1, $a2    # curdim < D  ?
    bltz $t0, ARASSNI5
    nop
    move $t2, $0         # t2 - индекс по элементам последнего измерения
ARASSNI3:
    sll $t4, $t2, 2
    add $t5, $a1, $t4
    lw $t0, 0($t5)
    add $t5, $a0, $t4
    sw $t0, 0($t5)
    addi  $t2, $t2, 1
    sub $t0, $t2, $t3
    bltz $t0, ARASSNI3
    nop
ARASSNI4:
    addi $t1, $t1, -1    # вправо или вверх
    beq $t1, $0, ARASSNIEND
    nop
    sll $t3, $t1, 2      # t3 = curdim * 4
    sub $t4, $gp, $t3
    lw $a0, -7996($t4)   # leftC0[curdim]
    lw $a1, -8016($t4)   # rightC0[curdim]
    lw $t5, -8036($t4)   # ind
    lw $t6, -4($a0)      # N
    addi $t5, $t5, 1
    sw $t5, -8036($t4)   # инд
    sub $t0, $t5, $t6    # ind < N?
    bltz $t0, ARASSNI1
    nop
    j ARASSNI4
    nop
ARASSNI5:                # вниз
    sll $t3, $t1, 2      # t3 = curdim * 4
    sub $t4, $gp, $t3
    sw $a0, -7996($t4)   # leftC0[curdim]
    sw $a1, -8016($t4)   # rightC0[curdim]
    sw $0,  -8036($t4)   # ind

    addi $t1, $t1, 1
    sll $t5, $t5, 2
    add $a0, $a0, $t5
    lw $a0, 0($a0)
    add $a1, $a1, $t5
    lw $a1, 0($a1)
    j ARASSNI1
    nop
ARASSNIEND:
    jr $ra
    nop

auxget:                   # порядок аргументов не такой, как в auxprint!
    addi $fp, $fp, -48    # a1 arg address
    sw $sp, 20($fp)       # a0 t
    move $sp, $fp
    sw $ra, 16($sp)

    sw $s0, 24($sp)
    sw $s1, 28($sp)
    sw $s2, 32($sp)
    sw $s3, 36($sp)
 
    addi $t0, $0, -1      # LINT
    bne  $a0, $t0, auxgetELSE2
	.rdata
	.align 2
auxgetSTRING1:
	.ascii "%i\0"
	.text
	.align 2
	lui $t1, %hi(auxgetSTRING1)
	addiu $a0, $t1, %lo(auxgetSTRING1)

    move  $s7, $sp
	addi  $sp, $fp, -16
    jal   scanf
	nop
	

	move  $sp, $s7
	j auxgetEND
	nop
	

auxgetELSE2:
	addi $t0, $0, -2      # LCHAR
	bne $a0, $t0, auxgetELSE3
	.rdata
	.align 2
auxgetSTRING2:
	.ascii "%c\0"
	.text
	.align 2
	lui $t1, %hi(auxgetSTRING2)
	addiu $a0, $t1, %lo(auxgetSTRING2)

	move  $s7, $sp
	addi  $sp, $fp, -16
	jal   scanf
	nop	

	move  $sp, $s7
	j auxgetEND
	nop

auxgetELSE3:
	addi $t0, $0, -3      # LFLOAT
	bne $a0, $t0, auxgetELSE4
	.rdata
	.align 2
auxgetSTRING3:
	.ascii "%f\0"
	.text
	.align 2
	lui $t1, %hi(auxgetSTRING3)
	addiu $a0, $t1, %lo(auxgetSTRING3)

	move  $s7, $sp
	addi  $sp, $fp, -16
	jal   scanf
	nop

	move  $sp, $s7
	j auxgetEND
	nop

auxgetELSE4:
	move $s0, $a1                 # array or struct address
	lui $t1, %hi(modetab)
	addiu $t1, $t1, %lo(modetab)  # t1 modetab
	sll   $t0, $a0, 2             # t0 type*4
	add   $t1, $t1, $t0           # t1 address of modetab[t]
	lw    $t0, 0($t1)             # t0 MARRAY or MSTRUCT or MPOINTER

	addi  $t2, $0, 1003           # MARRAY
	bne   $t0, $t2, auxgetELSE6
	nop

	lw    $s2, 4($t1)             # s2 type of elem
	move   $a0, $s2               # a0 type of elem
 	jal   szof                    # v0 d
	nop
	

	move  $s3, $v0                # s3 d
	lw    $t0, -4($s0)            # N
	mul   $t0, $s3, $t0
	add   $s1, $s0, $t0           # s1 address of a[N]

auxgetELSE5:
	sub   $t0, $s0, $s1
	bgez  $t0, auxgetEND

	move  $a0, $s2                # type of elem
	move  $a1, $s0                # address a[i]
	jal   auxget
	nop

	add   $s0, $s0, $s3
	j     auxprELSE5
	nop
	 
auxgetELSE6:
	addi  $t2, $0, 1002           # MSTRUCT
	bne   $t0, $t2, auxgetEND
	lw    $s1, 8($t1)             # s1 N (удвоенное число полей)
	addi  $s3, $0, 2              # s3 i
auxgetELSE7:
	sub  $t0, $s3, $s1            # i <= N
	bgtz $t0, auxgetEND
	sll  $t0, $s3, 2              # i*4
	add  $t2, $t1, $t0
	lw   $s2, 4($t2)              # type = modetab[t+i+1]

	move  $a0, $s2                # type
	move  $a1, $s0                # field address
	jal   auxget
	nop
	
	move $a0, $s2
	jal  szof
	nop

	add  $s0, $s0, $v0
	addi $s3, $s3, 2
	j auxprELSE7
	nop

auxgetEND:
    lw $s0, 24($sp)
    lw $s1, 28($sp)
    lw $s2, 32($sp)
    lw $s3, 36($sp)

	lw $ra, 16($sp)
	addi $fp, $sp, 48
	lw $sp, 20($sp)
	jr $ra
	nop

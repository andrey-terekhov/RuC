FUNC2:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal assert
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC6:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal asin
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC10:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal cos
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC14:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal sin
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC18:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal sin
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC22:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal log
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC26:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal log10
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC30:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal sqrt
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC34:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal rand
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC38:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal round
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC42:
	jr $ra
FUNC46:
	jr $ra
FUNC50:
	jr $ra
FUNC54:
	jr $ra
FUNC58:
	jr $ra
FUNC62:
	jr $ra
FUNC66:
	jr $ra
FUNC70:
	lw $v0,-1($a0)
	jr $ra
FUNC162:
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal exit
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
DEFARR1:
	sw $a1, 4($a0)
	li $v0, 4
	mul $v0, $v0, $a1
	sub $v0, $a0, $v0
	addi $v0, $v0, -4
	jr $ra
DEFARR2:
	sw $a0, 0($a2)
	move $t0, $ra
	jal DEFARR1
	move $ra, $t0
	addi $a2, $a2, -4
	addi $a0, $v0, -4
	addi $a3, $a3, -1
	bne $a3, $0, DEFARR2
	jr $ra

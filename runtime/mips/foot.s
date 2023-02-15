FUNC2:                          # assert()
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal assert
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC6:                          # asin
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal asin
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC10:                         # cos
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal cos
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC14:                         # sin
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal sin
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC18:                         # sin
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal sin
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC22:                         # log
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal log
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC26:                         # log10
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal log10
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC30:                         # sqrt
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal sqrt
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC34:                         # rand
	sw $ra, -4($sp)
	addi $sp, $sp, -4
	jal rand
	addi $sp, $sp, 4
	lw $ra, -4($sp)
	jr $ra
FUNC38:                         # round
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
	jr $ra

# defarr1
# объявление одномерного массива
# $a0 -- адрес первого элемента
# $a1 -- размер измерения
DEFARR1:
	sw $a1, 4($a0)              # Сохранение границы
	li $v0, 4                   # Загрузка размера слова
	mul $v0, $v0, $a1           # Подсчёт размера первого измерения массива в байтах
	sub $v0, $a0, $v0           # Считаем адрес после конца массива, т.е. $v0 -- на слово ниже последнего элемента
	addi $v0, $v0, -4
	jr $ra

# defarr2
# объявление многомерного массива, но сначала обязана вызываться процедура DEFARR1
# $a0 -- адрес первого элемента
# $a1 -- размер измерения
# $a2 -- адрес первого элемента предыдущего измерения
# $a3 -- размер предыдущего измерения
DEFARR2:
	sw $a0, 0($a2)              # Сохраняем адрес в элементе предыдущего измерения
	move $t0, $ra               # Запоминаем $ra, чтобы он не затёрся
	jal DEFARR1                 # Выделение памяти под массив
	move $ra, $t0               # Восстанавливаем $ra
	addi $a2, $a2, -4           # В $a2 следующий элемент в предыдущем измерении
	addi $a0, $v0, -4           # В $a0 первый элемент массива в текущем измерении, плюс выделяется место под размеры
	addi $a3, $a3, -1           # Уменьшаем счётчик
	bne $a3, $0, DEFARR2        # Прыгаем, если ещё не всё выделили
	jr $ra
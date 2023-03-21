	.section .mdebug.abi32      # ?
	.previous                   # следующая инструкция будет перенесена в секцию, описанную выше
	.nan    legacy;             # ?
	.module fp=xx               # ?
	.module nooddspreg          # ?
	.abicalls                   # ?
	.option pic0                # как если бы при компиляции была включена опция "-fpic" (что означает?)
	.text                       # последующий код будет перенесён в текстовый сегмент памяти
	.align 2                    # выравнивание последующих данных / команд по границе, кратной 2^n байт (в данном случае 2^2 = 4)

	.extern assert

	.extern asin                # Математические функции из стандартной библиотеки libc
	.extern cos
	.extern sin
	.extern exp
	.extern log
	.extern log10
	.extern sqrt
	.extern round

	.extern exit

	.globl main                 # делает метку main глобальной -- её можно вызывать извне кода (например, используется при линковке)
	.ent main                   # начало процедуры main
	.type main, @function       # тип "main" -- функция

main:
	lui $gp, %%hi(__gnu_local_gp)               # инициализация gp
	addiu $gp, $gp, %%lo(__gnu_local_gp)        # "__gnu_local_gp" -- локация в памяти, где лежит Global Pointer
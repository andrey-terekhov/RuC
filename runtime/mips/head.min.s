	.section .mdebug.abi32
	.previous
	.nan    legacy;
	.module fp=xx
	.module nooddspreg
	.abicalls
	.option pic0
	.text
	.align 2
	.globl main
	.ent main
	.type main, @function
main:
	lui $gp, %%hi(__gnu_local_gp)
	addiu $gp, $gp, %%lo(__gnu_local_gp)

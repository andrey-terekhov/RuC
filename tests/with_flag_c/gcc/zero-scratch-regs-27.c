/* { dg-do compile { target *-*-linux* } } */
/* { dg-options "-O2 -fzero-call-used-regs=all-gpr-arg" } */

int 
foo (int x)
{
  return x;
}

/* { dg-final { scan-assembler "xorl\[ \t\]+%edx, %edx" } } */
/* { dg-final { scan-assembler "movl\[ \t\]+%edx, %ecx" } } */
/* { dg-final { scan-assembler "movl\[ \t\]+%edx, %esi" { target { ! ia32 } } } } */
/* { dg-final { scan-assembler "movl\[ \t\]+%edx, %edi" { target { ! ia32 } } } } */
/* { dg-final { scan-assembler "movl\[ \t\]+%edx, %r8d" { target { ! ia32 } } } } */
/* { dg-final { scan-assembler "movl\[ \t\]+%edx, %r9d" { target { ! ia32 } } } } */

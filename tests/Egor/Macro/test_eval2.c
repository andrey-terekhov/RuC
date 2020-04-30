 
using bincompl;
process Neuronet =
begin
in  inp(int(15), int(15), int(15), int(15));
out ans(int(15));
-- 8 4 9 3 2 3

data JT, JK, JD, J, II: uint(10),
irot, layer: uint(4),
tab: [0..15:uint(4)]int(15), 
bram: [0..830: uint(10)][0..1023: uint(10)]int(15)



,in0
,in1
,in2
,in3
,in4
,in5
,in6
,in7
,in8

,s0
,s1
,s2
,s3


 
, r0x0
, r0x1
, r0x2
, r1x0
, r1x1
, r1x2
, r2x0
, r2x1
, r2x2
, r3x0
, r3x1
, r3x2

: int(15);
init
{
JT:=0 | JK:=0 | JD:=0 | II:=0 | J:=0 | irot:=0 | layer:=1
| s0:=0
| s1:=0
| s2:=0
| s3:=0

}

inp(a,b,c,d)
{if JT < 16 then
tab[JT]:=a | tab[JT+1]:=b | tab[JT+2]:=c | tab[JT+3]:=d | JT:=JT + 4

elif JK < 12 then
bram[II][JK]:=a | bram[II+1][JK]:=b | bram[II+2][JK]:=c | bram[II+3][JK]:=d | II:=II+4 |
if II == 8 then II:=0 | JK:=JK + 1 else II:=II+4 fi 

elif JD < 8
in0 := in4 | in1 := in5 | 
in2 := in6 | in3 := in7 |
in4 := a | in5 := b | in6 := c | in7 := d | JD:=JD+4

else
while layer < 3
do
while irot < 3
do
 
r0x0:=in0 |
r1x0:=in0 |
r2x0:=in0 |
r3x0:=in0 |
r0x1:=in1 |
r1x1:=in1 |
r2x1:=in1 |
r3x1:=in1 |
r0x2:=in2 |
r1x2:=in2 |
r2x2:=in2 |
r3x2:=in2 |


in0:=in3 |
in1:=in4 |
in2:=in5 |
in3:=in6 |
in4:=in7 |
in5:=in8 |






 
a0x0 = bram[0][J] |
a1x0 = bram[1][J] |
a2x0 = bram[2][J] |
a3x0 = bram[3][J] |
a0x1 = bram[4][J] |
a1x1 = bram[5][J] |
a2x1 = bram[6][J] |
a3x1 = bram[7][J] |
a0x2 = bram[8][J] |
a1x2 = bram[9][J] |
a2x2 = bram[10][J] |
a3x2 = bram[11][J] |

skip;
 
r0x0 := let x = sext(r0x0,30) * sext(a0x0,30) in x{29} || x{4:17} |
r1x0 := let x = sext(r1x0,30) * sext(a1x0,30) in x{29} || x{4:17} |
r2x0 := let x = sext(r2x0,30) * sext(a2x0,30) in x{29} || x{4:17} |
r3x0 := let x = sext(r3x0,30) * sext(a3x0,30) in x{29} || x{4:17} |
r0x1 := let x = sext(r0x1,30) * sext(a0x1,30) in x{29} || x{4:17} |
r1x1 := let x = sext(r1x1,30) * sext(a1x1,30) in x{29} || x{4:17} |
r2x1 := let x = sext(r2x1,30) * sext(a2x1,30) in x{29} || x{4:17} |
r3x1 := let x = sext(r3x1,30) * sext(a3x1,30) in x{29} || x{4:17} |
r0x2 := let x = sext(r0x2,30) * sext(a0x2,30) in x{29} || x{4:17} |
r1x2 := let x = sext(r1x2,30) * sext(a1x2,30) in x{29} || x{4:17} |
r2x2 := let x = sext(r2x2,30) * sext(a2x2,30) in x{29} || x{4:17} |
r3x2 := let x = sext(r3x2,30) * sext(a3x2,30) in x{29} || x{4:17} |

first:=0 | irot:= irot + 1 | J:=J + 1;

 

 r0x0:=r0x0 + r0x1 |


 r1x0:=r1x0 + r1x1 |


 r2x0:=r2x0 + r2x1 |


 r3x0:=r3x0 + r3x1 |









skip;

 r0x0:=r0x0 + r0x2 |


 r1x0:=r1x0 + r1x2 |


 r2x0:=r2x0 + r2x2 |


 r3x0:=r3x0 + r3x2 |

skip;
s0:= s0 + r0x0 |
s1:= s1 + r1x0 |
s2:= s2 + r2x0 |
s3:= s3 + r3x0 |

skip
done;
irot:=0 |
 
let s = 
if s0 < ({-4}:int(15)) then tab[0]
elif s0 > ({4}: int(15)) then tab[15]
else tab[s0{3:6}]
fi in
in0:=s |
let s = 
if s1 < ({-4}:int(15)) then tab[0]
elif s1 > ({4}: int(15)) then tab[15]
else tab[s1{3:6}]
fi in
in1:=s |
let s = 
if s2 < ({-4}:int(15)) then tab[0]
elif s2 > ({4}: int(15)) then tab[15]
else tab[s2{3:6}]
fi in
in2:=s |
let s = 
if s3 < ({-4}:int(15)) then tab[0]
elif s3 > ({4}: int(15)) then tab[15]
else tab[s3{3:6}]
fi in
in3:=s |
layer:= layer + 1
done;
JD:=0 | inform ans(s0)
end;

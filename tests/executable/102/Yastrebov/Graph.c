void main() 
{ 
char scr[50][50]; 
int i,j; 
int n; 
float y; 
for (i=0;i<50;i++) 
for (j=0;j<50;j++) 
scr[i][j]='a'; 
for (i=0;i<50;i++) 
{ y=i; 
y=sin(y/50)*50; 
n=round(y); 
scr[i][n]='*'; 
} 
printid (scr); 
}
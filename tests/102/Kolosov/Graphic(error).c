int fn(float p)
{int e;
p=p+0.5;
while (e<p) e++;
return e;
}


void main ()
{
char c[60][30];
int k,t,i;
float a,b;

for ( i=0;i<30;i++) {
                  for (k=0;k<60;k++) c[k][i]=' ';
                  }

               for ( i=0;i<60;i++) {
               	                  a=(i*5)/60;
               							t=fn( 15*sin(a));
                                    if ((t<=15)&&(t+15<29)) c[i][t+15]='X';
                                          						}



			for (i=0;i<60;i++) print(c[i]);	

}
//Miller A.
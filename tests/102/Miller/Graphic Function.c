int fn(float p)
 {
  int e=0;
  p=p+0.5;
  if (p>=0) 
   {
    e=0;
    while (e<p) e++; 
    return e;
   }
    else 
   {
    e=0;
    while (e>p) e--;
    return e;
   }	
 } 
void main ()
{
 char mas[30];
 char c[60][30];
 int k,t,i;
 float a,b,m,n=0;
 for ( i=0;i<30;i++) 
  {
   for (k=0;k<60;k++) c[k][i]=' ';
  }
 for ( i=0;i<60;i++) 
  {
   n=i*5;
   a=((n)/60);	                                 					
   t=fn( 15*sin(a));
   if ((t<=15)&&(t+15<29)) c[i][t+15]='X';
   for (k=0;k<60;k++)  
    {
     for (i=29;i>=0;i--) 
      {
       mas[i]=c[k][i]; 
      }
     printid(mas);
    }	
}
//Miller A.
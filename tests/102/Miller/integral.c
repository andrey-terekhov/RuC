float a,b,s,s1,c,e=1e-6;
int i,k,t,n;
void main()
{
 print("Введите нижнюю границу");
 getid(a);
 print("Введите верхнюю границу");
 getid(b);
 n=10;
 s=0; 
 s1=0;
 do 
  {
   c=(b-a)/n; 
   for (i=0;i<n;i++) 
    {
     s=s+((sin(a+i*c)+sin(a+(i+1)*c))/2)*c;
    }
   if (abs(s-s1)>e) 
    { 
     s1=s;
     s=0;
     n=n*2;
    }
  } 
 while(abs(s-s1)>e);
 print("Значение Интеграла\n");
 print(s);
 print("Итерации\n");
 print(n);
}
//Miller A.
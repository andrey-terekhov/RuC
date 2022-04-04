int a[10];
void main()
{
 int n,i,m;
 print("Введите массив");
 getid(a);
 n=a[0];
 for ( i=0;i<10;i++)
     if (a[i]>n)
         n=a[i];
 print("Максимум\n");
 print(n);
}
//Miller A.

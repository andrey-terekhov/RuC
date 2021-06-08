int a[10];
void main()
{
 int n,i,r,m;
 print("������� ������");
 getid(a);
 do
  {
   m=0;
   for ( r=0;r<9;r++)  
    if (a[r]>a[r+1])
     {
      n=a[r];
      a[r]=a[r+1];
      a[r+1]=n;
      m=1;
     }
  }
 while (m==1);
 print("�����������
 print(a);
}
//Miller A.
int a[5] = {5, 4, 3, 2, 1}, n = 5, i, j, t;
int main()
{
    for (j=1; j<n; j++)
      for(i=0; i<n-j; i++)
         if(a[i]>a[i+1])
         {
            t=a[i];
            a[i]=a[i+1];
            a[i+1]=t;
         }
   printid(a);    // 1 2 3 4 5
   return 0;
}

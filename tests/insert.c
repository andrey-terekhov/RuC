int a[13] = {0, 5, 3, 7, 5, 3, 7, -2, 9, 100, 0, 0, 0}, beg = 1, res, end=10;
int insert(int val, int val1)
  {
    int cur = beg, pred = beg;
    while (cur)
    {
         if(a[pred] == val1)
         {
             a[end+1] = val;
             a[end+2] = a[pred+1];
             a[pred+1] = end+1;
             return end;
         }
         pred = cur;
         cur = a[cur+1];
     }
     return 0;
  }
void main()
{
    printid(a);
    if (insert(-100,-2))
        printid(a);
}


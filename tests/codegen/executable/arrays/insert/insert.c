int a[13] = {0, 5, 3, 7, 5, 3, 7, -2, 9, 100, 0, 0, 0}, beg = 1, res, end=10;

int insert(int val, int val1)
  {
    int cur = beg, pred = beg;
    while (cur)
    {
      if(a[pred] == val1)
      {
          a[end + 1] = val;
          a[end + 2] = a[pred + 1];
          a[pred + 1] = end + 1;
          return end;
      }
      pred = cur;
      cur = a[cur + 1];
     }
     return 0;
  }


void main()
{
    assert(a[0] == 0, "a[0] must be 0");
    assert(a[1] == 5, "a[1] must be 5");
    assert(a[2] == 3, "a[2] must be 3");
    assert(a[3] == 7, "a[3] must be 7");
    assert(a[4] == 5, "a[4] must be 5");
    assert(a[5] == 3, "a[5] must be 3");
    assert(a[6] == 7, "a[6] must be 7");
    assert(a[7] == -2, "a[7] must be -2");
    assert(a[8] == 9, "a[8] must be 9");
    assert(a[9] == 100, "a[9] must be 100");
    assert(a[10] == 0, "a[10] must be 0");
    assert(a[11] == 0, "a[11] must be 0");
    assert(a[12] == 0, "a[12] must be 0");

  // и что делает insert?
    if (insert(-100,-2))
    {
      printid(a);
    }
}
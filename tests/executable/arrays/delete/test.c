int a[11] = {0, 5, 3, 7, 5, 3, 7, -2, 9, 100, 0}, beg = 1, res;

int find(int val)
{
    int i = beg;
    while(i)
	{
        if(a[i] == val)
            return i;
        i = a[i + 1];
	}
    return 0;
}

int delete (int val)
{
    int cur, prev = beg;
    if(a[beg] == val)
    {
        beg = a[beg + 1];
        return prev;
    }
    cur = a[beg + 1];
    while(cur)
    {
        if(a[cur] == val)
        {
            a[prev + 1] = a[cur + 1];
            return cur;
        }
        prev = cur;
        cur = a[cur + 1];
    }
    return 0;
}


void main()
{
    assert(find(5) == 1, "find(5) must be 1");
    assert(find(100) == 9, "find(100) must be 9");
    assert(find(-2) == 7, "find(-2) must be 7");
	res = delete(7);
    assert(res == 3, "res must be 3");
    res = delete(5);
    assert(res == 1, "res must be 1");
	res = delete(100);
    assert(res == 9, "res must be 9");
}

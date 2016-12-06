int a[11] = {0,5, 3, 7, 5, 3, 7, -2, 9, 100, 0}, beg = 1, res;
int find(int val)
{
    int i = beg;
    while(i)
	{
        if(a[i] == val)
            return i;
        i = a[i+1];
	}
    return 0;
}
int delete (int val)
{
    int cur, prev = beg;
    if(a[beg] == val)
    {
        beg = a[beg+1];
        return prev;
    }
    cur = a[beg+1];
    while(cur)
    {
        if(a[cur] == val)
        {
            a[prev+1] = a[cur+1];
            return cur;
        }
        prev = cur;
        cur = a[cur+1];
    }
    return 0;
}
void main()
{
    print(find(5));     // 1
    print(find(100));   // 9
    print(find(-2));    // 7
	res = delete(7);
	printid(res);       // 3
    res = delete(5);    // 1
	printid(res);
	res = delete(100);  // 9
	printid(res);

}

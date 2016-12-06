int a[11] = {0,5, 3, 7, 5, 3, 7, -2, 9, 100, 0}, beg = 1, res;
//должна выдать 9
int find(int val)
{
    int i = beg;
    while(i)
        if(a[i] == val)
            return i;
		else
            i = a[i+1];
    return 0;
}
void main()
{
	res = find(100);
	printid(res);
}

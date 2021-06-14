void main()
{
    int i, j;
    struct lines 
    {
        int *x; int *y;
    } *l1;

    struct lines l;
   
    int x = 20;
    int y = 20;
    
    l1 = &l;
    l.x = &x;
    l.y = &y;
    
    y = 21;
	y = (l1 -> x);
    print(*l.y);         
    print("end");         
}

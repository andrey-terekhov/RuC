void main()
{
    int i, j;
    struct point {int x; int y;} *l1;
    
    struct point p, p1 = {20, 21};
    l1 =& p1;
     
    assert(p1.x == 20, "p1.x must be 20");
	assert(p1.y == 21, "p1.y must be 21");
    
    p1.y = 5;
    
    l1 -> y = 6;
    l1 -> x = 7;
    
    assert(l1->x == 7, "l1->x must be 7");
	assert(l1->y == 6, "l1->y must be 6");        
}

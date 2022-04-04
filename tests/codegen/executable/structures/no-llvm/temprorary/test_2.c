void main()
{
    int i, j;
    struct lines {struct point {int x; int y;} p1; struct point p2;} *l1,
    l3[1] = {{{15, 16}, {17, 18}}};
    
    struct lines s4 = {{10, 11}, {12, 13}};
    
    int a[3] = {1, 2, 3},
    c[2][3] =  {{1, 2, 3}, {4, 5, 6}};
    
    struct point p, p1 = {20, 21};
    int b = s4.p2.y;
    
    assert(l3[0].p1.x == 15, "l3[0].p1.x must be 15");
    assert(l3[0].p1.y == 16, "l3[0].p1.y must be 16");
    assert(l3[0].p2.x == 17, "l3[0].p1.x must be 17");
    assert(l3[0].p2.y == 18, "l3[0].p1.y must be 18");

    assert(s4.p1.x == 10, "s4.p1.x must be 10");
    assert(s4.p1.y == 11, "s4.p1.y must be 11");
    assert(s4.p2.x == 12, "s4.p1.x must be 12");
    assert(s4.p2.y == 13, "s4.p1.y must be 13");

    assert(a[0] == 1, "a[0] must be 1");
    assert(a[1] == 2, "a[1] must be 2");
    assert(a[2] == 3, "a[2] must be 3");

    assert(b == 13, "b must be 13");

    assert(c[0][0] == 1, "c[0][0] must be 1");
    assert(c[0][1] == 2, "c[0][1] must be 2");
    assert(c[0][2] == 3, "c[0][2] must be 3");
    assert(c[1][0] == 4, "c[1][0] must be 4");
    assert(c[1][1] == 5, "c[1][1] must be 5");
    assert(c[1][2] == 6, "c[1][2] must be 6");
    
	p = s4.p1;
    
    assert(p.x == 10, "p.x must be 10");
    assert(p.y == 11, "p.y must be 11");

    assert(p1.x == 20, "p1.x must be 20");
    assert(p1.y == 21, "p1.y must be 21");
    
	l3[0].p2.y = 5;
    
    assert(l3[0].p1.x == 15, "l3[0].p1.x must be 15");
    assert(l3[0].p1.y == 16, "l3[0].p1.y must be 16");
    assert(l3[0].p2.x == 17, "l3[0].p1.x must be 17");
    assert(l3[0].p2.y == 5, "l3[0].p1.y must be 5");
    
	s4.p2 = p;
    
    assert(s4.p1.x == 10, "s4.p1.x must be 10");
    assert(s4.p1.y == 11, "s4.p1.y must be 11");
    assert(s4.p2.x == 10, "s4.p1.x must be 10");
    assert(s4.p2.y == 11, "s4.p1.y must be 11");
    
	l1 = &s4;
    l1->p1 = p1;
	l1->p2 = p;
    
    assert(l1->p1.x == 20, "l1->p1.x must be 20");
    assert(l1->p1.y == 21, "l1->p1.y must be 21");
    assert(l1->p2.x == 10, "l1->p1.x must be 10");
    assert(l1->p2.y == 11, "l1->p1.y must be 11");

    assert(s4.p1.x == 20, "s4.p1.x must be 20");
    assert(s4.p1.y == 21, "s4.p1.y must be 21");
    assert(s4.p2.x == 10, "s4.p1.x must be 10");
    assert(s4.p2.y == 11, "s4.p1.y must be 11");
}


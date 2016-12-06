void main()
{
    int i, j;
    struct line {struct point {int x; int y;} p1,p2;} *l1,
    l3[1] = {{{15,16}, {17, 18}}};
    
    struct line s4 = {{10,11}, {12,13}};
    
    int a[3] = {1, 2, 3},
    c[2][3] =  {{1,2,3}, {4, 5, 6}};
    
    struct point p, p1 = {20, 21};
    int b = s4.p2.y;
    
    printid(l3);         // 15, 16, 17, 18
    printid(s4);         // 10, 11, 12 13
    printid(a);          // 1 2 3
    printid(b);          // 13
    printid(c);          // 1, 2, 3, 4, 5, 6
    
	p = s4.p1;
    
    printid(p);          // 10, 11
    printid(p1);         // 20,21
    
	l3[0].p2.y = 5;
    
    printid(l3);         // 15, 16, 17, 5
    
	s4.p2 = p;
    
    printid(s4);         // 10, 11, 10, 11
    
	l1 = &s4;
    l1->p1 = p1;
	l1->p2 = p;
    
    print(*l1);          // 20, 21, 10, 11
    printid(s4);         // 20, 21, 10, 11
}


void main()
{
    int i, j;
    struct line {struct point {int x; int y;} p1; struct point p2;} *l1,
    l3[1] = {{{15,16}, {17, 18}}};
    
    struct line s4 = {{10,11}, {12,13}};
    
    int a[3] = {1, 2, 3},
    c[2][3] =  {{1,2,3}, {4, 5, 6}};
    
    struct point p, p1 = {20, 21};
    int b = s4.p2.y;
    
    print(l3);         // 15, 16, 17, 18
    print(s4);         // 10, 11, 12 13
    print(a);          // 1 2 3
    print(b);          // 13
    print(c);          // 1, 2, 3, 4, 5, 6
    
	p = s4.p1;
    
    print(p);          // 10, 11
    print(p1);         // 20,21
    
	l3[0].p2.y = 5;
    
    print(l3);         // 15, 16, 17, 5
    
	s4.p2 = p;
    
    print(s4);         // 10, 11, 10, 11
    
	l1 = &s4;
    l1->p1 = p1;
	l1->p2 = p;
    
    print(*l1);        // 20, 21, 10, 11
    print(s4);         // 20, 21, 10, 11
}


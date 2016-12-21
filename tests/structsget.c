void main()
{
    int i, j;
    struct line {struct point {int x; int y;} p1; struct line p2;} *l1,
    l3[1] = {{{15,16}, {17, 18}}};
    
    struct line s4 = {{10,11}, {12,13}};
    
    int a[3] = {1, 2, 3},
    c[2][3] =  {{1,2,3}, {4, 5, 6}};
    
    struct point p, p1 = {20, 21};
    int b = s4.p2.y;
    printid(b);
    getid(b);
    printid(b);
    
    printid(l3);
    getid(l3);         // 15, 16, 17, 18
    printid(l3);
    
    printid(s4);
    getid(s4);         // 10, 11, 12 13
    printid(s4);
    
    printid(a);
    getid(a);          // 1 2 3
    printid(a);
    
    printid(b);
    getid(b);          // 13
    printid(b);
    
    printid(c);
    getid(c);          // 1, 2, 3, 4, 5, 6
    printid(c);
}


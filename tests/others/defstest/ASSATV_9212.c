void main()
{
    int i, j;
    struct point {int x; int y;} *l1;
    
    struct point p, p1 = {20, 21};
    l1=&p1;
    
    print("p1 20 21");         
    printid(p1);      
    
   p1.y = 5;
    
    
    l1->y =6;
    l1->x =7;
    
    print("7 6");         
    print(*l1);         
}

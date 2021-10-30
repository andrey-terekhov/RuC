void main()
{
    int a = 2, b = 3, c;
    int d[2] = {4, 2};
    float m[2] = {1.5, 2.5};
    float k;
   
    c = a * ++b;
    print ("c  8");
    printid(c);

    c = a * b++;
    print("c  8");
    printid(c);
    
    print("b  5");
    printid(b);

    c = d[0]++;
    print ("c  4");
    printid(c);
    
    c = d[0];
    print ("c  5");
    printid(c);

    k = m[0]--;
    print ("k  1.500000");
    printid(k);
    
    k = m[0];
    print ("k  0.500000");
    printid(k);

    k = ++m[1];
    print ("k  3.500000");
    printid(k);

    c = --d[1];
    print ("c  1");
    printid(c);
}
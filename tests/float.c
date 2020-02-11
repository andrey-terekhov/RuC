void main()
{
    float x[1];
    double y = .01, z;
    x[0] = 3.14;
    print(x[0]+y); // 3.15
    print(x[0]-y); // 3.13
    print(x[0]*y); // 0.0314
    print(x[0]/y); // 314
    print("\n");
    
    x[0]+=y;       // 3.15
    printid(x);
    x[0]-=y;       // 3.14
    printid(x);
    x[0]*=y;       // 0.0314
    printid(x);
    x[0]/=y;       // 3/14
    printid(x);
    print("\n");
    
    z = x[0]+=y;   // 3.15
    printid(z);
    z = x[0]-=y;   // 3.14
    printid(z);
    z = x[0]*=y;   // 0.0314
    printid(z);
    z = x[0]/=y;   // 3.14
    printid(z);



    
}

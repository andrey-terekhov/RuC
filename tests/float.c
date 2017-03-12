void main()
{
    float x[1];
    double y = .01, z;
    x[0] = 3.14;
    print(x[0]+y);
    print(x[0]-y);
    print(x[0]*y);
    print(x[0]/y);
    print("\n");
    
    x[0]+=y;
    printid(x);
    x[0]-=y;
    printid(x);
    x[0]*=y;
    printid(x);
    x[0]/=y;
    printid(x);
    print("\n");
    
    z = x[0]+=y;
    printid(z);
    z = x[0]-=y;
    printid(z);
    z = x[0]*=y;
    printid(z);
    z = x[0]/=y;
    printid(z);



    
}

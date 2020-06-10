int MAIN()
{
    int i=8;
    int *p=&i;
    i=*p<<=2;
 
    print(*p);   
    print(i);   
        
    return 0;
}

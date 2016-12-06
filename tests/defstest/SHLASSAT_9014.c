int MAIN()
{
    int i=8;
    int *p=&i;
    i=*p<<=2;
 
    print("i ",*p);   
    print(i);   
        
    return 0;
}

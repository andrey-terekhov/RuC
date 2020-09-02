void main()
{
     struct somestr {int a;int b;} *pointstr;
    
     struct somestr  str={1,2};
    
    pointstr= &str;
    
    print("1 2");        
    print(*pointstr);        
}

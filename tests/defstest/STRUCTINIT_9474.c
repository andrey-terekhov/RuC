void main()
{
     struct somestr {int a;int b;} *pointstr;
    
     struct somestr  str={2,2};
    
    pointstr= &str;
    
    print("2 2");
    print(*pointstr);        
}

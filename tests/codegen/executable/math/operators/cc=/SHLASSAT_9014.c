int MAIN()
{
    int i = 8;
    int *p = &i;
    i = *p <<= 2;
 
    assert(*p == 32, "*p must be 32");
    assert(i == 32, "i must be 32"); 
        
    return 0;
}

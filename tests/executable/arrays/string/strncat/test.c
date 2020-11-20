void MAIN()
{
    char a = 'a';
    char b[] = "\t1";
    int i;

    printid(a);
    a = '\n';
    printid(a);

    strcat(&b, "bc");
    printid(b);
    
    a = '\0';
    i = a;
    printid(i);
}

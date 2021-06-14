void main()
{
    int i;
    char s1[] = "aqwertyui";
    char s2[] = "bwerty";

    printid(s1);
    printid(s2);
    strcat(&s1, s2);
    
    printid(s1);
    strcat(&s1, "123");
    printid(s1);

    
}

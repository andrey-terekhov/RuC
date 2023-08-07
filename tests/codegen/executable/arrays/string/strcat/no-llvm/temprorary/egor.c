void main()
{
    int i;
    char s1[] = "aqwertyui";
    char s2[] = "bwerty";

    printid(s1);
    printid(s2);
    char s3[] = strcat(s1, s2);
    
    printid(s3);
    char s4[] = strcat(s1, "123");
    printid(s4);

    
}

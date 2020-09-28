void main()
{
    char c[2][] = {"abc", "defg"};
    printid(c);

    struct{int i; int j;} s = {11, 22};
   
    printid(s);
    
    struct students{char names[5]; int mark;} st[] =
    { 
        {"stud1", 1}, 
        {"stud2", 3}
    };

    printid(st);   
}

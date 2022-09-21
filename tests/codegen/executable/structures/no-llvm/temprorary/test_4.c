void main()
{
    char c[2][] = {"abc", "defg"};

    assert(c[0][0] == 'a', "c[0][0] must be 'a'");
    assert(c[0][1] == 'b', "c[0][1] must be 'b'");
    assert(c[0][2] == 'c', "c[0][2] must be 'c'");
    assert(c[1][0] == 'd', "c[1][0] must be 'd'");
    assert(c[1][1] == 'e', "c[1][1] must be 'e'");
    assert(c[1][2] == 'f', "c[1][2] must be 'f'");
    assert(c[1][3] == 'g', "c[1][3] must be 'g'");

    {
        struct{int i; int j;} s = {11, 22};

        assert(s.i == 11, "s.i must be 11");
        assert(s.j == 22, "s.j must be 22");
        
        {   
            struct students{char names[5]; int mark;} st[] =
            { 
                {"stud1", 1}, 
                {"stud2", 3}
            };

            assert(st[0].names[0] == 's', "st[0].names[0] must be 's'");
            assert(st[0].names[1] == 't', "st[0].names[1] must be 't'");
            assert(st[0].names[2] == 'u', "st[0].names[2] must be 'u'");
            assert(st[0].names[3] == 'd', "st[0].names[3] must be 'd'");
            assert(st[0].names[4] == '1', "st[0].names[4] must be '1'");
            assert(st[0].mark == 1, "st[0].mark must be 1");

            assert(st[1].names[0] == 's', "st[1].names[0] must be 's'");
            assert(st[1].names[1] == 't', "st[1].names[1] must be 't'");
            assert(st[1].names[2] == 'u', "st[1].names[2] must be 'u'");
            assert(st[1].names[3] == 'd', "st[1].names[3] must be 'd'");
            assert(st[1].names[4] == '2', "st[1].names[4] must be '2'");
            assert(st[1].mark == 3, "st[1].mark must be 3");
        }  
    } 
}

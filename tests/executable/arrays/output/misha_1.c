void main() 
{
	char s[] = "asdf";
    char s2[2][] = {"ad", "qwer"};

    assert(s[0] == 'a', "s[0] must be a");
    assert(s[1] == 's', "s[1] must be s");
    assert(s[2] == 'd', "s[2] must be d");
    assert(s[3] == 'f', "s[3] must be f");

    assert(s2[0][0] == 'a', "s2[0][0] must be a");
    assert(s2[0][1] == 'd', "s2[0][1] must be d");
    assert(s2[1][0] == 'q', "s2[1][0] must be q");
    assert(s2[1][1] == 'w', "s2[1][1] must be w");
    assert(s2[1][2] == 'e', "s2[1][2] must be e");
    assert(s2[1][3] == 'r', "s2[1][3] must be r");
}

int main()
{
    char ch = '1';
    int i = 7;
    
    assert(ch == '1', "Something wrong with char!");
    
    assert(i, "int i = 7 not true!");
    
    assert(1 < 0, "Should be fail!");

    return 0;
}


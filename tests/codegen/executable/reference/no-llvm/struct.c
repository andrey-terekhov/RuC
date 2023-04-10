struct k 
{
    int a;
    int b;
};

void main()
{
    k a = {1,2};
    k& b = a;
    b.b = 5;
    assert(a.b == 5, "Error assigning to field of reference to struct. a.b must be 5");
    b.b++;
    assert(a.b == 6, "Error using unary increment on field of reference to struct. a.b must be 6");
}
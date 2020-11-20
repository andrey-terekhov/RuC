void* f(void* arg)
{
    print("thread 1\n");
    return arg;
}

void main()
{
    t_create(f, 0);
    print("thread 0\n");
    t_join(1);
}

void* threadf(void* x)
{
    int i = 0;
    print("Thread1 alive!\n");
    t_exit();
    return 0;
}

int main() {
    t_create(threadf, 0);
    print("Thread0 alive!\n");
    t_join(1);
    return 0;
}

void* threadf(void* x)
{
    int i = 113;

    print("Thread 1 alive!\n");
    assert(i == 113, "i must be 113");

    t_exit();
    return 0;
}

int main() {
    t_create(threadf);

    print("Thread 0 alive!\n");

    t_join(1);
    return 0;
}

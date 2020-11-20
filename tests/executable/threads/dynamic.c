void* threadf(void* x)
{
    int i = 113;
    print("Thread 1 alive!\n");
    print(i);
    t_exit();
    return 0;
}

int main() {
    t_create(threadf);
    print("Thread 0 alive!\n");
    t_join(1);
    return 0;
}

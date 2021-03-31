// Проверка на присвоедние Never NULL pointera к указетелю

int main() {
    int num = 5;
    int &pointer = &num;
    int *ptr = pointer;
    return 0;
}
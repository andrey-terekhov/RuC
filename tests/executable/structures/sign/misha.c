struct write{
    int x;
    int a[2][3];
};

int main() {
    struct write w;
    struct write *wp = &w;
    int i= 0, j = 0;
    w.x = 5;
    for (i = 0; i < 2; ++i)
        for (j = 0; j < 3; ++j)
            w.a[i][j] = i + j;
    
    print(wp->a[1]);
	return 0;
}

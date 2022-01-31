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

    assert(wp->a[0][0] == 0, "wp->a[0][0] must be 0");
    assert(wp->a[0][1] == 1, "wp->a[0][1] must be 1");
    assert(wp->a[0][2] == 2, "wp->a[0][2] must be 2");

    assert(wp->a[1][0] == 1, "wp->a[1][0] must be 1");
    assert(wp->a[1][1] == 2, "wp->a[1][1] must be 2");
    assert(wp->a[1][2] == 3, "wp->a[1][2] must be 3");

	return 0;
}

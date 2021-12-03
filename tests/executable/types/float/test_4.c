float a[2] = {2, 2}, b[3] = {3, 3, 3}, c = 4, d = 5;
int i = 0, j = 1;

void main()
{
    ++b[i];

    assert(b[0] == 4, "b[0] must be 4");

    ++c;

    assert(c == 5, "c must be 5");

    d = ++b[j];

    assert(d == 4, "d must be 4");

    d = ++c;

    assert(d == 6, "d must be 6");

    --b[1];

    assert(b[1] == 3, "b[1] must be 3");

    --i;

    assert(i == -1, "i must be -1");

    c = --b[j];

    assert(c == 2, "c must be 2");

    i = 1; j = 2;
    c = --d;

    assert(c == 5, "c must be 5");

    b[i]++;

    assert(b[1] == 3, "b[1] must be 3");

    d++;

    assert(d == 6, "d must be 6");

    d = b[j]++;

    assert(d == 3, "d must be 3");

    c = d++;

    assert(c == 3, "c must be 3");

    b[1] += i;

    assert(b[1] == 4, "b[1] must be 4");

    --d;

    assert(d == 3, "d must be 3");

    d = b[j]--;

    assert(d == 4, "d must be 4");

    d = c--;

    assert(d == 3, "d must be 3");
    assert(c == 2, "c must be 2");
}

int white = 100, black = 62;
void main()
{
    int value = black;
    setmotor(3, 50);
    setmotor(4, 50);
    while (value < white)
    {
        value = getansensor(6);
        sleep(1);
    }
    setmotor(3, 0);
    setmotor(4, 0);
}

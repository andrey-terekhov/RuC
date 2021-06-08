int heater = 1, debit1 = 2, debit2 = 3;
int tmin = 30, tmax = 60;             // градусов
int P = 100, k = 3;                   // P - стандартная мощность мотора, k - коэффициент управляющего воздействия
void main()
{
    t_create_direct                          // 1 - нить регулярного опроса датчика температуры
    while (1)
    {
        int val = getdigsensor(heater, 1);   // это датчик нагрева
        t_msg_send({3, val});
        t_sleep(1000);                // 1 секунда
    }
    t_exit

    t_create_direct                          // 2 - нить регулярного опроса датчиков расхода воды s1 и s2
    while (1)
    {
        int val = getdigsensor(debit1, 2);
        t_msg_send({4, val});         // s1
        val = getdigsensor(debit2, 3);
        t_msg_send({4, val});         // s2

        t_sleep(50);                  // 50 миллисекунд
    }
    t_exit

    t_create_direct                          // 3 - нить нагрева
    struct msg_info{int numTh; int data;} msg;
    while (1)
    {
        msg = t_msg_receive();
        if (msg.data < tmin)
            setmotor(1, 0);               // считаю, что это команда включить нагреватель
        else if (msg.data > tmax)
            setmotor(1, 1);               // считаю, что это команда выключить нагреватель
        t_sleep(1000);
    }
    t_exit

    t_create_direct                          // 4 измерение циркуляции воды
    struct message{int numTh; int data;} msg;
    int s1, s2, U;
    while (1)
    {
        msg = t_msg_receive();
        s1 = msg.data;
        msg = t_msg_receive();
        s2 = msg.data;
        U = s1 - s2;
        setmotor(2, P-k*U);
        setmotor(2, P+k*U);
    }
    t_exit
}


int COUNT_PHILOSOPHERS = 5;
int COUNT_EATING = 42;

/* char names[][] =
{
	"Aristotle",
	"Schopenhauer",
	"Nietzsche",
	"Ortega y Gasset",
	"Ilyin",
};
 */
char names[5] = {'1', '2', '3', '4', '5'};

void* philosopher(void* n)
{
    int num = t_getnum() - 1, count = 0;
    print(" sat down at the table\n");
    print(names[num]);
	while (count++ < COUNT_EATING)
	{
        print(" is thinking about the great\n");
        print(names[num]);
		t_sem_wait(num);
		t_sem_wait((num + 1) % COUNT_PHILOSOPHERS);
        print(" eats pasta ");
        print(names[num]);
		t_sem_post((num + 1) % COUNT_PHILOSOPHERS);
		t_sem_post(num);
	}	
    print(" ate ");
    print(names[num]);
    print(" times and was satisfied");
    printid(COUNT_EATING);
	return 0;
}


int main()
{
    int i;
	
	for (i = 0; i < COUNT_PHILOSOPHERS; ++i)
	{ t_sem_create(1); }
	for (i = 0; i < COUNT_PHILOSOPHERS; ++i)
	{ t_create(philosopher, 0); }
	for (i = 1; i <= COUNT_PHILOSOPHERS; ++i)
	{ t_join(i); }

	return 0;
}

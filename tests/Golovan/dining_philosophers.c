int COUNT_PHILOSOPHERS = 5;
int COUNT_EATING = 42;
/*
 char names[5][] =
{
	"Aristotle",
	"Schopenhauer",
	"Nietzsche",
	"Ortega y Gasset",
	"Ilyin"
};
*/
char names[5] = {'0', '1', '2', '3', '4'};

void* philosopher(void* n)
{
    int num = t_getnum() - 1, count = 0;
    printf("%c sat down at the table\n", names[num]);
	while (count++ < COUNT_EATING)
	{
        printf("%c is thinking about the great count = %i\n", names[num], count);
		t_sem_wait(num);
		t_sem_wait((num + 1) % COUNT_PHILOSOPHERS);
        printf(" eats pasta %c\n", names[num]);
		t_sem_post((num + 1) % COUNT_PHILOSOPHERS);
		t_sem_post(num);
	}	
    printf(" %c ate %i times and was satisfied\n", names[num], count-1);
	return 0;
}

int main()
{
    int i, numthread, numsem;
	
	for (i = 0; i < COUNT_PHILOSOPHERS; ++i)
	{ numsem = t_sem_create(1);
    }
	for (i = 0; i < COUNT_PHILOSOPHERS ; ++i)
	{ numthread = t_create(philosopher);
    }
	for (i = 1; i <= COUNT_PHILOSOPHERS; ++i)
	{ t_join(i);
    }

	return 0;
}

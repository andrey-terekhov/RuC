цел главная()
{
	struct vector {int size; int a[1];};
	struct vector a, b;
	struct vector *c = &a;
		
	struct mas {struct vector vects[2];};
	struct mas masa[3];
	
	int i = 0;	
		
    a.size = 5;
    b.size = 6;
		
	//for (i = 0; i < a.size; ++i)
//		a.a[i] = i + 1, b.a[i] = a.a[i] + 2;	
	
	masa[0].vects[0] = a;
	masa[0].vects[1] = b;
	
	masa[1].vects[0] = b;
	masa[1].vects[1] = a;	
	
	assert(masa[0].vects[0].size == 5, "masa[0].vects[0].size must be 5");
	assert(masa[0].vects[1].size == 6, "masa[0].vects[1].size must be 6");
	assert(masa[1].vects[0].size == 6, "masa[1].vects[0].size must be 6");
	assert(masa[1].vects[1].size == 5, "masa[1].vects[1].size must be 5");

	возврат 0;
}

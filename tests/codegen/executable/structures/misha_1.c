struct write 
{
	int a;
	int b;
	int c;
	int x[10];
};


int main() 
{
	int i;
	struct write w;
	w.a = w.b = w.c = 5;
	
	for (i = 0; i < 10; ++i)
		w.x[i] = i * i;

	assert(w.a == 5, "w.a must be 5");
	assert(w.b == 5, "w.b must be 5");
	assert(w.c == 5, "w.c must be 5");
	assert(w.x[0] == 0, "w.x[0] must be 0");
	assert(w.x[1] == 1, "w.x[1] must be 1");
	assert(w.x[2] == 4, "w.x[2] must be 4");
	assert(w.x[3] == 9, "w.x[3] must be 9");
	assert(w.x[4] == 16, "w.x[4] must be 16");
	assert(w.x[5] == 25, "w.x[5] must be 25");
	assert(w.x[6] == 36, "w.x[6] must be 36");
	assert(w.x[7] == 49, "w.x[7] must be 49");
	assert(w.x[8] == 64, "w.x[8] must be 64");
	assert(w.x[9] == 81, "w.x[9] must be 81");

	return 0;
}
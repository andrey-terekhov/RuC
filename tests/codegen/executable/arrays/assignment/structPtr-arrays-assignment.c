struct myS {
	int a;
	int array[3];
	int multiArray[2][3];
};

void main()
{
	int a[3] = {2, 3, 4};
	int b[2][3] = {{1, 2, 3}, {1, 2, 3}};
	myS q;
	myS* f = &q;
	f->array = {1, 2, 3};
	f->multiArray = {{1, 2, 3}, { 2, 3, 4 } };

	assert(f->array[0] == 1, "f.array[0] must be 1");
	assert(f->array[1] == 2, "f.array[1] must be 2");
	assert(f->array[2] == 3, "f.array[2] must be 3");

	assert(f->multiArray[0][0] == 1, "f.multiArray[0][0] must be 1");
	assert(f->multiArray[0][1] == 2, "f.multiArray[0][1] must be 2");
	assert(f->multiArray[0][2] == 3, "f.multiArray[0][2] must be 3");
	assert(f->multiArray[1][0] == 2, "f.multiArray[1][0] must be 2");
	assert(f->multiArray[1][1] == 3, "f.multiArray[1][1] must be 3");
	assert(f->multiArray[1][2] == 4, "f.multiArray[1][2] must be 4");

	f->array = a;
	f->multiArray = {a, {4, 5, 6}};

	assert(f->array[0] == 2, "f.array[0] must be 2");
	assert(f->array[1] == 3, "f.array[1] must be 3");
	assert(f->array[2] == 4, "f.array[2] must be 4");

	assert(f->multiArray[0][0] == 2, "f.multiArray[0][0] must be 2");
	assert(f->multiArray[0][1] == 3, "f.multiArray[0][1] must be 3");
	assert(f->multiArray[0][2] == 4, "f.multiArray[0][2] must be 4");
	assert(f->multiArray[1][0] == 4, "f.multiArray[1][0] must be 4");
	assert(f->multiArray[1][1] == 5, "f.multiArray[1][1] must be 5");
	assert(f->multiArray[1][2] == 6, "f.multiArray[1][2] must be 6");

	f->array = b[0];
	f->multiArray = {b[0], a};

	assert(f->array[0] == 1, "f.array[0] must be 1");
	assert(f->array[1] == 2, "f.array[1] must be 2");
	assert(f->array[2] == 3, "f.array[2] must be 3");

	assert(f->multiArray[0][0] == 1, "f.multiArray[0][0] must be 1");
	assert(f->multiArray[0][1] == 2, "f.multiArray[0][1] must be 2");
	assert(f->multiArray[0][2] == 3, "f.multiArray[0][2] must be 3");
	assert(f->multiArray[1][0] == 2, "f.multiArray[1][0] must be 2");
	assert(f->multiArray[1][1] == 3, "f.multiArray[1][1] must be 3");
	assert(f->multiArray[1][2] == 4, "f.multiArray[1][2] must be 4");

	f->multiArray = {{2, 3, 4}, f->array};

	assert(f->multiArray[0][0] == 2, "f.multiArray[0][0] must be 2");
	assert(f->multiArray[0][1] == 3, "f.multiArray[0][1] must be 3");
	assert(f->multiArray[0][2] == 4, "f.multiArray[0][2] must be 4");
	assert(f->multiArray[1][0] == 1, "f.multiArray[1][0] must be 1");
	assert(f->multiArray[1][1] == 2, "f.multiArray[1][1] must be 2");
	assert(f->multiArray[1][2] == 3, "f.multiArray[1][2] must be 3");

	f->multiArray[0] = f->array;

	assert(f->multiArray[0][0] == 1, "f.multiArray[0][0] must be 1");
	assert(f->multiArray[0][1] == 2, "f.multiArray[0][1] must be 2");
	assert(f->multiArray[0][2] == 3, "f.multiArray[0][2] must be 3");
	assert(f->multiArray[1][0] == 1, "f.multiArray[1][0] must be 1");
	assert(f->multiArray[1][1] == 2, "f.multiArray[1][1] must be 2");
	assert(f->multiArray[1][2] == 3, "f.multiArray[1][2] must be 3");

	f->multiArray[0] = { 2, 3, 4 };

	assert(f->multiArray[0][0] == 2, "f.multiArray[0][0] must be 2");
	assert(f->multiArray[0][1] == 3, "f.multiArray[0][1] must be 3");
	assert(f->multiArray[0][2] == 4, "f.multiArray[0][2] must be 4");
	assert(f->multiArray[1][0] == 1, "f.multiArray[1][0] must be 1");
	assert(f->multiArray[1][1] == 2, "f.multiArray[1][1] must be 2");
	assert(f->multiArray[1][2] == 3, "f.multiArray[1][2] must be 3");

	f->array = f->multiArray[0];

	assert(f->array[0] == 2, "f.array[0] must be 2");
	assert(f->array[1] == 3, "f.array[1] must be 3");
	assert(f->array[2] == 4, "f.array[2] must be 4");

	b = {f->array, f->multiArray[0]};

	assert(b[0][0] == 2, "b[0][0] must be 2");
	assert(b[0][1] == 3, "b[0][1] must be 3");
	assert(b[0][2] == 4, "b[0][2] must be 4");
	assert(b[1][0] == 2, "b[1][0] must be 2");
	assert(b[1][1] == 3, "b[1][1] must be 3");
	assert(b[1][2] == 4, "b[1][2] must be 4");
}
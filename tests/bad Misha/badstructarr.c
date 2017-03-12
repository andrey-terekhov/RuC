struct write {
	int a;
	int b;
	int c;
	int x[10];
};

int main() {
	int i;
	struct write w;
	w.a = w.b = w.c = 5;
	for (i = 0; i < 10; ++i)
		w.x[i] = i * i;
		
	print(w);
	return 0;
}
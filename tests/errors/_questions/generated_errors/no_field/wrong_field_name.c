struct id
{
	char lastname[20];   
	char firstname[20]; 
	int year;
};


int main()
{
	struct id structura;
	structura.lstname[1] = 'D';
	//rr если раскомментить, то дальнейший код будет корректно обрабатываться
	return 0;
}
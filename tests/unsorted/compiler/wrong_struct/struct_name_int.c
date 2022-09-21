int main()
{
	int a;

	struct int
	//ff // если раскоментить, то дальше код будет корректно считываться, и выдаст только необходимую ошибку
	{
		char lastname[20];   
		char firstname[20]; 
		int year;
	};
	return 0;
}
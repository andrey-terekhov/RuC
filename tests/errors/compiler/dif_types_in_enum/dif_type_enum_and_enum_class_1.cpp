enum En1
{
	a,
	b,
	c
};

enum class En2
{
	a,
	b,
	c
};

int main()
{
	enum En1 en1 = En2::b;
	return 0;
}

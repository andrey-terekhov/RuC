#define _name123_ 1
#define name123 1
#define _ 1

int main()
{
	assert(_name123_, "_name123_");
	assert(name123, "name123");
	assert(_, "_");

	return 0;
}

void main ()
{
	struct types {int arr[7];} obj;
	obj.arr[0] = 1;

	assert(obj.arr[0] == 1, "obj.arr[0] must be 1");
}
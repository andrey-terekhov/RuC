#macro macro_1
	#define A 0
	#define B 9
	assert(C == 6, "assert in macro");
#endm

#macro macro_2
	#set B B + 10
#endm

#macro macro_3
	#define C 6
	#macro macro_4
		#define C A + B
	#endm
#endm

int main()
{
	macro_3
	macro_1
	assert(B == 9, "define in macro");

	macro_4
	assert(C == 9, "define in macro in macro");

	macro_2
	assert(B == 19, "set in macro");

	macro_4
	assert(C == 19, "define in macro in macro after set in macro");

	return 0;
}

int A = 0;
int B = 0;
int C = 0;
int D = 0;
int E = 0;
int F = 0;
int G = 0;
int H = 0;
int I = 0;
int J = 0;
int K = 0;
int L = 0;

#define/*long comment*/A 1
#define B/*long comment*/1
#define/*long
comment*/C 1
#define D/*long
comment*/1

#define /*long comment*/E 1
#define F /*long comment*/1
#define /*long
comment*/G 1
#define H /*long
comment*/1

#define/*long comment*/ I 1
#define J/*long comment*/ 1
#define/*long
comment*/ K 1
#define L/*long
comment*/ 1

int main()
{
	assert(A, "long comment before ident without separators");
	assert(B, "long comment before value without separators");
	assert(C, "long comment before ident without separators");
	assert(D, "long comment before value without separators");

	assert(E, "long comment before ident with separator before comment");
	assert(F, "long comment before value with separator before comment");
	assert(G, "long comment before ident with separator before comment");
	assert(H, "long comment before value with separator before comment");

	assert(I, "long comment before ident with separator after comment");
	assert(J, "long comment before value with separator after comment");
	assert(K, "long comment before ident with separator after comment");
	assert(L, "long comment before value with separator after comment");

	return 0;
}

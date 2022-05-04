
#define X_COMPASS 3

#define Y_COMPASS 4

#define Z_COMPASS 5
#define FI_COMPASS 6


#define D0 0

int fi;
int x;
int y;
int z;

int main()
{
	
	x = getdigsensor(X_COMPASS, { D0, D0 });
	y = getdigsensor(Y_COMPASS, { D0, D0 });
	z = getdigsensor(Z_COMPASS, { D0, D0 });
	fi = getdigsensor(FI_COMPASS, { D0, D0 });

	return 0;

}

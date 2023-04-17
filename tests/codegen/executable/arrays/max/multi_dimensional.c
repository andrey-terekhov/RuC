int sizes[4] = {3, 2, 2, 3};
int  a[3] = {1, 2, 3};
int  b[2][3] =  {a, {4, 5, 6}};
int  c[2][2][3] = {{{7, 8, 9}, {10, 11, 12}}, b};
int  d[3][2][2][] = {c, c, c};
int  numOfLowestDimensions[3] = {12, 6, 3};

int max_value_d()
{
	int index = 0;
	int max = 0;
	for (int i = 0; i < sizes[0]; i++)
	{
		for (int j = 0; j < sizes[1]; j++)
		{
			for (int k = 0; k < sizes[2]; k++)
			{
				for (int l = 0; l < sizes[3]; l++)
				{
					if (d[i][j][k][l] > max)
					{
						max = d[i][j][k][l];
						index = i * numOfLowestDimensions[0] + j * numOfLowestDimensions[1] + k * numOfLowestDimensions[2] + l;
					}
				}
			}
		}
	}


	return max;
}

int max_index_d()
{
	int index = 0;
	int max = 0;
	for (int i = 0; i < sizes[0]; i++)
	{
		for (int j = 0; j < sizes[1]; j++)
		{
			for (int k = 0; k < sizes[2]; k++)
			{
				for (int l = 0; l < sizes[3]; l++)
				{
					if (d[i][j][k][l] > max)
					{
						max = d[i][j][k][l];
						index = i * numOfLowestDimensions[0] + j * numOfLowestDimensions[1] + k * numOfLowestDimensions[2] + l;
					}
				}
			}
		}
	}


	return index;
}

void main()
{
	assert(max_value_d() == 12, "max value in d array == 12");
	assert(max_index_d() == 29, "last max value in d array is on index 29");
}
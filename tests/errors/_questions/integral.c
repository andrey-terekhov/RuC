
struct polynomial_function
{
        int deg;
        float* coef;
};

struct polynomial_function new_polynomial_function(int length, float* elements)
{
        struct polynomial_function result;

        result.deg = length;
        result.coef = (float*) malloc((length+1)*sizeof(float));

        for (int i = 0; i < length + 1; ++i)
        {
                result.coef[i] = elements[i];
        }

        return result;
}

void free_polynomial_function(struct polynomial_function* f)
{
        f->deg = 0;
        free(f->coef);
}


float get_value(struct polynomial_function* f, float arg)
{
        float result = 0;

        for (int i = 0; i < f->deg + 1; ++i)
        {
                result += f->coef[i]*pow(arg, i);
        }

        return result;
}

float sum_of_diffs(struct polynomial_function* f, float a, float b, float h)
{
        float result = 0.0;

        for (float i = a; i < b; i += h)
        {
                result += fabsf(get_value(f, i + h) - get_value(f, i))*h;
        }

        return result;
}

float calculate_integral(struct polynomial_function* f, float a, float b, float h)
{
        float result = 0;

        for (float i = a; i < b; i += h)
        {
                result += get_value(f, i)*h;
        }

        return result;
}

float integrate_function(struct polynomial_function* f, float epsilon, float a, float b)
{
        float d = b - a;

        while (sum_of_diffs(f, a, b, d) > epsilon)
        {
                d = d/2.0;
        }

        return calculate_integral(f, a, b, d);
}

void test()
{
        float array1[] = {1.0, 1.0, 1.0};
        struct polynomial_function p = new_polynomial_function(2, array1);

        print("Test 1\nEpsilon value: 0.001\nFrom 0 to 1\nPolynom: ");
        print("Test value (calculated with wolframalpha): 1.8333\nCalculated value: ");
        printf("%f\n", integrate_function(&p, 0.001, 0.0, 1.0));
        free_polynomial_function(&p);

        float array2[] = {9.0, 4.5, 7.0, 16.4};
        p = new_polynomial_function(3, array2);

        print("\nTest 2\nEpsilon value: 0.01\nFrom 2 to 3\nPolynom: ");
       printf("Test value (calculated with wolframalpha): 331.083\nCalculated value: ");
        print(integrate_function(&p, 0.01, 2.0, 3.0));
        free_polynomial_function(&p);

        float array3[] = {-10.0};
        p = new_polynomial_function(0, array3);

        print("\nTest 3\n Epsilon value: 0.1\n From -2 to 2\n Polynom: ");
        print("Test value (calculated with wolframalpha): -40\n Calculated value: ");
        print(integrate_function(&p, 0.1, -2.0, 2.0));
        free_polynomial_function(&p);
}

int main(int argc, char const *argv[])
{
        test();
        return 0;
}

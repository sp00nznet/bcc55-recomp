/* x87 floating point: libm, conversions, printf formatting */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static double integrate(double (*f)(double), double a, double b, int n)
{
    double h = (b - a) / n, s = (f(a) + f(b)) / 2;
    int i;
    for (i = 1; i < n; i++) s += f(a + i * h);
    return s * h;
}

int main(void)
{
    float f = 1.0f / 3.0f;
    long double ld = 2.0L;
    printf("%.6f %.6f %.6f\n", sqrt(2.0), sin(1.0), exp(1.0));
    printf("%.10f\n", integrate(sin, 0.0, 3.14159265358979, 1000));
    printf("%g %e %.3Lf\n", (double)f, 12345.678, sqrtl(ld));
    printf("%d %ld %d\n", (int)-2.7, (long)1e9, (int)floor(-2.5));
    printf("%.4f %.4f\n", atof("3.25e2"), fmod(10.0, 3.0));
    printf("%.4f\n", pow(1.5, 7.25));
    return 0;
}

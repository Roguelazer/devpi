#include <stdio.h>

#define EPS (1e-18)

double modpow(long base, long exponent, long modulus)
{
	long result = 1;
	while (exponent > 0) {
		if (exponent & 1) {
			result = (result * base) % modulus;
		}
		exponent >>= 1;
		base = (base * base) % modulus;
	}
	return (double) result;
}

double pow16(long exp) {
	int i;
	long pos_exp = exp < 0 ? -exp : exp;
	double total = 1.0;
	if (exp == 0) {
		return 1.0;
	}
	for (i = 0; i < pos_exp; i++) {
		total *= 16.0;
	}
	if (exp < 0) {
		total = 1 / total;
	}
	return total;
}

double compute_term(long n, long j)
{
	double s = 0.0, t = 0.0;
	double newt;
	double pow;
	double diff;
	long k = 0;
	long r;

	/* left sum */
	while (k <= n) {
		r = 8*k + j;
		s = s + modpow(16, n-k, r) / r;
		s -= (long) s; /* fmod by 1.0 */
		k++;
	}

	/* right sum */
	k = n + 1;
	while (1) {
		pow = pow16(n-k);
		newt = t + pow / ((k << 3) + j);
		diff = t - newt;
		diff = diff < 0 ? -diff : diff;
		if (diff < EPS) {
			break;
		} else {
			t = newt;
		}
		k++;
	}
	return s + t;
}

int compute_digit(long n) {
	double s;
    s = 4.0 * compute_term(n, 1);
    s -= 2.0 * compute_term(n, 4);
    s -= compute_term(n, 5);
    s -= compute_term(n, 6);

	/* fmod s by 1.0 */
	while (s < 0) { s++; }
	while (s > 1) { s--; }

	return s * 16;
}

int main (int argc, char **argv) {
	int i;
	int d;
	for (i = 0; i < 10; i++) {
		printf("%x", compute_digit(i));
	}
	puts("");
	return 0;
}

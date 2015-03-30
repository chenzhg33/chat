#include <string.h>
void zero_del(char *msg, int len) {
	int i = 0, j = 0;
	while (j < len) {
		*(msg + i) = *(msg + j);
		++i;
		j += 2;
	}
}
void zero_add(const char *omsg, int len, char *nmsg) {
	int i = 0, j = 0;
	while (i < len) {
		*(nmsg + j) = *(omsg + i);
		++i;
		j += 2;
	}
	nmsg[len << 1] = 0;
}

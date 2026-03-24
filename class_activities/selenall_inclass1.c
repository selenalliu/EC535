#include <stdio.h>
#include <inttypes.h>

int main () {
    uint32_t inputNum;
    
    printf("Enter an unsigned integer: ");
    scanf("%u", &inputNum);

    int onesCount = 0;

    for (int i = 31; i >= 0; i--) {
        uint32_t numShift = inputNum >> i;
        printf("%d", numShift & 1);

        if (numShift & 1 == 1) {
            onesCount++;
        }
    }
    
    printf("\nNumber of 1s: %d\n", onesCount);

    return 0;
}

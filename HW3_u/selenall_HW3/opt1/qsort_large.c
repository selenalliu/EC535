#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define UNLIMIT
#define MAXARRAY 60000 /* this number, if too large, will cause a seg. fault!! */
// #define LOOP_COUNT 100 // selenall: added to increase the number of iterations for gprof data
#define DISTANCE(x, y, z) sqrt(((x)*(x)) + ((y)*(y)) + ((z)*(z))) // selenall: added to avoid using pow function, and to lower the number of function calls
#define COMPARE(v1, v2) ((v1)->distance > (v2)->distance) - ((v1)->distance < (v2)->distance)


struct my3DVertexStruct {
  int x, y, z;
  double distance;
};

int compare(const void *elem1, const void *elem2)
{
  /* D = [(x1 - x2)^2 + (y1 - y2)^2 + (z1 - z2)^2]^(1/2) */
  /* sort based on distances from the origin... */

  const struct my3DVertexStruct *v1 = (struct my3DVertexStruct *)elem1; // selenall: using pointer casting to avoid dereferencing the pointer multiple times (expensive)
  const struct my3DVertexStruct *v2 = (struct my3DVertexStruct *)elem2; // selenall: same as above
  return COMPARE(v1, v2); // selenall: using previously defined macro to avoid branching (with conditional moves) + lower function overhead
}


int main(int argc, char *argv[]) {
  
  // for (int j =0; j < LOOP_COUNT; j++) { // selenall: added to increase the number of iterations for gprof data
  struct my3DVertexStruct array[MAXARRAY];
  FILE *fp;
  int i,count=0;
  int x, y, z;


  if (argc<2) {
    fprintf(stderr,"Usage: qsort_large <file>\n");
    exit(-1);
  }
  else {
    fp = fopen(argv[1],"r");
    
    while((fscanf(fp, "%d", &x) == 1) && (fscanf(fp, "%d", &y) == 1) && (fscanf(fp, "%d", &z) == 1) &&  (count < MAXARRAY)) {
	 array[count].x = x;
	 array[count].y = y;
	 array[count].z = z;
	//  array[count].distance = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2)); // selenall: commented out
   array[count].distance = DISTANCE(x, y, z); // selenall: using previously defined macro to avoid using pow function, and to lower the number of function calls
	 count++;
    }
  }
  printf("\nSorting %d vectors based on distance from the origin.\n\n",count);

  
    qsort(array,count,sizeof(struct my3DVertexStruct),compare);
  
  for(i=0;i<count;i++)
    printf("%d %d %d\n", array[i].x, array[i].y, array[i].z);
  
  // } // selenall: part of outer loop used for testing/gprof purposes
  
  return 0;
}

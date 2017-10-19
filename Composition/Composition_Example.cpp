#include "Composition_Example.h"

void Composition_Example::composition(int num_ims, int res_size, int** im_red, int** im_green, int** im_blue, float** im_depth,
                                        int* c_red, int* c_green, int* c_blue,float* c_depth) {
     for (int i = 0; i < num_ims; i++) {
     	for (int j = 0; j < res_size; j++) {
     		if (im_depth[i][j] < c_depth[j]) {
     			c_depth[j] = im_depth[i][j];
				c_red[j] = im_red[i][j];
				c_green[j] = im_green[i][j];
				c_blue[j] = im_blue[i][j];
			}
     	}
     }
}

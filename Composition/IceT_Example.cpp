// miniGraphics is distributed under the OSI-approved BSD 3-clause License.
// See LICENSE.txt for details.
//
// Copyright (c) 2017
// National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
// the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
// certain rights in this software.

#include "IceT_Example.hpp"

//#define NUM_TILES_X 2
//#define NUM_TILES_Y 2
//#define TILE_WIDTH 300
//#define TILE_HEIGHT 300

// static void InitIceT();
// static void DoFrame();
// static void Draw();

// static int winId;
static IceTContext icetContext;

void IceT_Example::composition(int num_ims,
                               int res_size,
                               int** im_red,
                               int** im_green,
                               int** im_blue,
                               float** im_depth,
                               int* c_red,
                               int* c_green,
                               int* c_blue,
                               float* c_depth) {
  int rank, numProc;
  IceTCommunicator icetComm;

  char* myargv[1];
  int myargc = 1;

  /* Setup MPI. */
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);

  /* Setup an IceT context. Since we are only creating one, this context will
  * always be current. */
  icetComm = icetCreateMPICommunicator(MPI_COMM_WORLD);
  icetContext = icetCreateContext(icetComm);
  icetDestroyMPICommunicator(icetComm);

  icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
  icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_FLOAT);
  icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);
  icetDisable(ICET_COMPOSITE_ONE_BUFFER);

  IceTImage result =
      icetGetStateBufferImage(ICET_STRATEGY_BUFFER_0, 1, res_size);
  icetClearImage(result);

  //	float * foo1 = icetImageGetColorf(tt1);
  //	foo1[0] = 0.1;
  //	foo1[1] = 0.1;
  //	foo1[2] = 0.1;
  //	foo1[3] = 0.1;
  //	foo1[4] = 0.2;
  //	foo1[5] = 0.2;
  //	foo1[6] = 0.2;
  //	foo1[7] = 0.2;
  //
  //	float * doo1 = icetImageGetDepthf(tt1);
  //	doo1[0] = 0.5;
  //	doo1[1] = 0.5;

  //	icetComposite(tt1,tt2,0);
  //
  //	float * testc = icetImageGetColorf(tt1);
  //	float * testd = icetImageGetDepthf(tt1);
  //	cout << testc[0] << ' ' << testc[1] << ' ' << testc[2] << ' ' <<
  // testc[3] << ' ' << testd[0] << endl;
  //	cout << testc[4] << ' ' << testc[5] << ' ' << testc[6] << ' ' <<
  // testc[7] << ' ' << testd[1] << endl;

  for (int i = 0; i < num_ims; i++) {
    IceTImage temp =
        icetGetStateBufferImage(ICET_STRATEGY_BUFFER_1, 1, res_size);
    icetClearImage(temp);
    float* tempc = icetImageGetColorf(temp);
    float* tempd = icetImageGetDepthf(temp);
    for (int j = 0; j < res_size; j++) {
      tempc[4 * j] = im_red[i][j] / 256.0;
      tempc[4 * j + 1] = im_green[i][j] / 256.0;
      tempc[4 * j + 2] = im_blue[i][j] / 256.0;
      tempc[4 * j + 3] = 0.0;  // opacity
      tempd[j] = im_depth[i][j];
    }
    icetComposite(result, temp, 0);
  }

  float* resc = icetImageGetColorf(result);
  float* resd = icetImageGetDepthf(result);

  for (int k = 0; k < res_size; k++) {
    c_red[k] = resc[4 * k] * 256;
    c_blue[k] = resc[4 * k + 1] * 256;
    c_green[k] = resc[4 * k + 2] * 256;
    c_depth[k] = resd[k];
  }
}

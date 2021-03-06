/*
   This file is part of the Mandelbox program developed for the course
    CS/SE  Distributed Computer Systems taught by N. Nedialkov in the
    Winter of 2015 at McMaster University.

    Copyright (C) 2015 T. Gwosdz and N. Nedialkov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include "camera.h"
#include "renderer_para.h"
#include "mandelbox_para.h"

#define CAM_DATA_FILE "cam_params.dat"

void getParameters(char *filename, CameraParams *camera_params, RenderParams *renderer_params,
		   MandelBoxParams *mandelBox_paramsP);
void init3D       (CameraParams *camera_params, const RenderParams *renderer_params);
void renderFractal(const CameraParams &camera_params, const RenderParams &renderer_params, unsigned char* image);
void saveBMP      (const char* filename, const unsigned char* image, int width, int height);

MandelBoxParams mandelBox_params;

int main(int argc, char** argv)
{
  CameraParams    camera_params;
  RenderParams    renderer_params;
  int num_of_nodes, my_rank;
  int current_frame_num, next_frame;

  //Initialize MPI and get all necessary information
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_of_nodes);//Get the number of processors
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);//Get the rank of the current process

  //Open cam movement file
  FILE *cam_data_file;
  cam_data_file = fopen(CAM_DATA_FILE,"r");

  getParameters(argv[1], &camera_params, &renderer_params, &mandelBox_params);

  int image_size = renderer_params.width * renderer_params.height;
  unsigned char *image = (unsigned char*)malloc(3*image_size*sizeof(unsigned char));

  //Save the original file name (not sure why because the files are going to named f001.bmp etc)
  char file_name[80];
  strncpy(file_name, renderer_params.file_name, 80);

  current_frame_num = 0;
  next_frame = my_rank;

  while (!feof(cam_data_file)) {

    for (int i=0; i<3; i++) fscanf(cam_data_file, " %lf", &camera_params.camPos[i]);
    for (int i=0; i<3; i++) fscanf(cam_data_file, " %lf", &camera_params.camTarget[i]);
		for (int i=0; i<3; i++) fscanf(cam_data_file, " %lf", &camera_params.camUp[i]);

    //update filename of current frame
    sprintf(renderer_params.file_name, "%s%04d%s", "videos/f", current_frame_num, ".bmp");
	
	//Only compute the frame if the processes next frame matches the current frame
    if (next_frame == current_frame_num){
      init3D(&camera_params, &renderer_params);

      renderFractal(camera_params, renderer_params, image);

      saveBMP(renderer_params.file_name, image, renderer_params.width, renderer_params.height);

      printf("node %d rendered frame %d\n", my_rank, current_frame_num);
	  
	  //Increment the next frame for the process by the number of processes 
      next_frame += num_of_nodes;
    }
	
    current_frame_num++;
   }

  free(image);
  fclose(cam_data_file);
  MPI_Finalize();

  return 0;
}

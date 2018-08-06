#ifndef VAR_COMP_HPP_
#define VAR_COMP_HPP_

#include <stdlib.h>
#include <stdint.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string>

using namespace cv;
using namespace std;

typedef uint32_t bus_t;
typedef uint32_t data_t;

typedef uint8_t pixel_t;
typedef int8_t  pixel_t_s;

struct comp_t {
  //header packet
  uint32_t size;
  uint32_t avg;
  uint32_t frame_width;
  uint32_t data_width;

  //data
  bus_t * data;
};

template<typename T>
int find_msb(T data);

template<typename T>
T sign_extend(T data, int size);

string type2str(int type); 
  
int get_num_frames(Mat image, int frame_width, int frame_height);

void get_frame_pack(comp_t &frame_comp, Mat &frame);

void delete_compressed_image(comp_t * image_comp, int num_frames);

Mat uncompress_image(comp_t * image_comp, int num_frames);

Mat uncompress_frame(comp_t &frame);

float get_error(Mat img_original, Mat img);

double get_compression_ratio(Mat image, comp_t * image_comp, int num_frames);

comp_t * get_compressed_image(Mat &image, int frame_width, int frame_height);

#endif

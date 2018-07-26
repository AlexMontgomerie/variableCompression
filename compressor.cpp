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

struct comp_t {
  bus_t avg;
  bus_t data_width;
  bus_t * data;
  int size;
}; 

template<typename T>
int find_msb(T data)
{
  int data_size =sizeof(T)*8; 
  for(int i=data_size-1;i>=0;i--)
  {
    if(data & (1 << i))
      return i;
  }
  return 0;
}

string type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

int get_compressed_image(comp_t *& image_comp, Mat image, int frame_width, int frame_height);
void get_frame_pack(Mat frame, comp_t * frame_comp);
float get_compression_ratio(Mat image, comp_t * image_comp, int num_frames);

int main()
{

  //load in an image
  //Mat img(800, 800, OPENCV_TYPE, Scalar(100, 250, 30)); 
  Mat img_tmp = imread("med-test.jpg");
  Mat img;
  cvtColor(img_tmp,img,COLOR_BGR2GRAY);
  if (img.empty()) 
  {
    cout << "Could not open or find the image" << endl;
    cin.get(); //wait for any key press
    return -1;
  }

  namedWindow("test");
  imshow("test",img);
  waitKey(0);
  destroyWindow("test");

  cout << "Image Loaded ..." << endl;
  string type =type2str(img.type()); 
  cout << "Matrix: " << type <<" " << img.cols << " x " << img.rows << " x " << img.channels() << endl;

  //create frame buffer
  comp_t * image_comp = NULL;
  cout << "Compressed Image Buffer Created ..." << endl;

  //get frame buffer
  int num_frames = get_compressed_image(image_comp, img, 50, 50);

  if (!num_frames)
    cout << "ERROR: image of wrong dimensions" << endl;

  float cr = get_compression_ratio(img,image_comp,num_frames);
  cout << "compression ratio: " << cr <<endl;

  //completed script
  return 0;

}

//function to test size and get compression ratio
float get_compression_ratio(Mat image, comp_t * image_comp, int num_frames)
{
  //get original size
  int original_size = (int) sizeof(pixel_t)*image.cols*image.rows;
  //get size compressed
  int comp_size = 0;
  for(int i=0; i<num_frames;i++) {
    comp_size += (int) sizeof(bus_t)*(2+image_comp[i].size);
  }
  cout << "original size: " << original_size << " comp size: " << comp_size << endl;

  //return compression ratio
  return (float) (original_size/comp_size);
}

int get_compressed_image(comp_t *& image_comp, Mat image, int frame_width, int frame_height)
{
  int image_width  = image.cols;
  int image_height = image.rows;
  
  //check frame size are suitable
  if(image_width%frame_width != 0)
    return 0;
  if(image_height%frame_height != 0)
    return 0;

  int num_frames = (int) ( (image_width/frame_width) * (image_height/frame_height) );
  cout <<"num frames : " << num_frames <<endl;  

  //create space for frames
  *image_comp = new comp_t [num_frames];

  int array_index = 0;

  for(int x=0;x<image_width;x+=frame_width) {
    for(int y=0;y<image_height;y+=frame_height) {
      Mat frame = image(Range(x,x+frame_width),Range(y,y+frame_height));
      get_frame_pack(frame,(*image_comp)[array_index]);
      //get_frame_pack(frame);
      array_index++;
    }
  }

  //return number of frames
  return num_frames;
}

void get_frame_pack(Mat frame, comp_t * frame_comp)
{

  //number of pixels in frame
  int num_pixels = frame.cols * frame.rows;

  //get mean
  frame_comp.avg = (bus_t) mean(frame)[0];
  

  //get max data size
  bus_t max_size = 1;
  for(int i=0;i<frame.rows;i++) {
    for(int j=0;j<frame.cols;j++) {
      pixel_t pixel = abs( frame.at<pixel_t>(i,j) - (pixel_t) frame_comp.avg );
      int msb = find_msb<pixel_t>(pixel);
      if(msb > max_size)
        max_size = msb;
    }
  }

  //save the width of new data
  max_size = max_size + 1;
  frame_comp.data_width = max_size;
  
  //save the size of the data
  int num_pixels_comp = (int) ceil(num_pixels * max_size / (sizeof(bus_t)*8) );
  frame_comp.size = num_pixels_comp;

  pixel_t mask = ( 1 << (max_size+1) ) - 1; 

  cout << num_pixels_comp << endl;
  
  //cout << "max_size" << max_size << " mask " << (int) mask << endl;

  //create buffer for data
  frame_comp.data = new bus_t [num_pixels_comp];

  int frame_data_index = 0;
  int curr_shift = 0;

  //clear buffer
  for(int i=0;i<num_pixels_comp;i++)
    frame_comp.data[i] = 0;

  //pack data
  for(int i=0;i<frame.rows;i++) {
    for(int j=0;j<frame.cols;j++) {
      
      //get pixel
      pixel_t pixel = abs( frame.at<pixel_t>(i,j) - (pixel_t) frame_comp.avg );
      pixel &= mask;
      
      //store data
      frame_comp.data[frame_data_index] |= (pixel << curr_shift);
      curr_shift += max_size;

      //check for overflow
      if( curr_shift > (sizeof(bus_t)*8-1) )
      {
        curr_shift = curr_shift - sizeof(bus_t)*8;
        frame_data_index++;
        frame_comp.data[frame_data_index] |= (pixel >> (max_size - curr_shift) );
      } 
    }
  }

  return frame_comp;

}

Mat uncompress_frame(comp_t frame)
{

}

Mat uncompress_image(comp_t * image, int num_frames)
{

}


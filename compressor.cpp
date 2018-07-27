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

int get_num_frames(Mat image, int frame_width, int frame_height)
{
  int image_width  = image.cols;
  int image_height = image.rows;
  //check frame size are suitable
  if(image_width%frame_width != 0)
    return 0;
  if(image_height%frame_height != 0)
    return 0;

  return (int) ( (image_width/frame_width) * (image_height/frame_height) );
}

comp_t * get_compressed_image(Mat image, int frame_width, int frame_height);
comp_t get_frame_pack(Mat frame);
double get_compression_ratio(Mat image, comp_t * image_comp, int num_frames);
void delete_compressed_image(comp_t * image_comp, int num_frames);
Mat uncompress_image(comp_t * image_comp, int num_frames);
Mat uncompress_frame(comp_t frame);

int main()
{

  int frame_width  = 20;
  int frame_height = 20;

  //load in an image
  //Mat img(800, 800, OPENCV_TYPE, Scalar(100, 250, 30));
  Mat img_tmp = imread("big-test.jpg");
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
  int num_frames = get_num_frames(img, frame_width, frame_height);
  if (!num_frames)
  {
    cout << "ERROR: image of wrong dimensions" << endl;
    return -1;
  }

  image_comp = get_compressed_image(img, frame_width, frame_height);


  double comp_ratio = get_compression_ratio(img,image_comp,num_frames);
  printf("compression ratio: %f\n",comp_ratio);

  //Try uncompressing
  Mat img_out = uncompress_image(image_comp, num_frames);

  delete_compressed_image(image_comp, num_frames);

  //completed script
  return 0;

}

//function to test size and get compression ratio
double get_compression_ratio(Mat image, comp_t * image_comp, int num_frames)
{
  //get original size
  float original_size = (int) sizeof(pixel_t)*image.cols*image.rows;
  //get size compressed
  float comp_size = 0;
  for(int i=0; i<num_frames;i++) {
    comp_size += (int) sizeof(bus_t)*(2+image_comp[i].size);
  }
  cout << "original size: " << original_size << " comp size: " << comp_size << endl;

  //return compression ratio
  return (float) (original_size/comp_size);
}

comp_t * get_compressed_image(Mat image, int frame_width, int frame_height)
{
  //get dimensions
  int image_width  = image.cols;
  int image_height = image.rows;

  //get number of frames
  int num_frames = get_num_frames(image, frame_width, frame_height);
  if(!num_frames)
    return NULL;

  cout <<"num frames : " << num_frames <<endl;

  //create space for frames
  comp_t * image_comp = new comp_t [num_frames];

  int array_index = 0;

  for(int x=0;x<image_width;x+=frame_width) {
    for(int y=0;y<image_height;y+=frame_height) {
      Mat frame = image(Range(x,x+frame_width),Range(y,y+frame_height));
      image_comp[array_index] = get_frame_pack(frame);
      //get_frame_pack(frame);
      array_index++;
    }
  }

  //return number of frames
  return image_comp;
}

comp_t get_frame_pack(Mat frame)
{

  //create new frame
  comp_t frame_comp;

  //number of pixels in frame
  int num_pixels = frame.cols * frame.rows;
  //cout << "num pixels: " << num_pixels << endl;

  //get mean
  frame_comp.avg = (bus_t) mean(frame)[0];

  //get max data size
  bus_t max_size = 1;
  for(int i=0;i<frame.rows;i++) {
    for(int j=0;j<frame.cols;j++) {
      pixel_t pixel = frame.at<pixel_t>(i,j) - (pixel_t) frame_comp.avg ;
      int msb = find_msb<pixel_t>(abs((pixel_t_s) pixel));
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

  pixel_t mask = ( 1 << (max_size) ) - 1;

  //cout << "num_pixels_comp: " << num_pixels_comp << endl;

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
      //cout << "index: "<< frame_data_index << endl;
      //get pixel
      pixel_t pixel = abs( frame.at<pixel_t>(i,j) - (pixel_t) frame_comp.avg );
      pixel &= mask;

      //check indexing is correct
      if(frame_data_index >= num_pixels_comp)
        return frame_comp;

      //store data
      frame_comp.data[frame_data_index] |= (pixel << curr_shift);
      curr_shift += max_size;

      //check for overflow
      if( curr_shift >= (sizeof(bus_t)*8) )
      {
        //shift index around
        curr_shift = curr_shift - sizeof(bus_t)*8;

        //check indexing is correct
        if(frame_data_index >= num_pixels_comp)
          return frame_comp;

        if(curr_shift!=0)
        {
          frame_data_index++;
          frame_comp.data[frame_data_index] |= (pixel >> (max_size - curr_shift) ); //TODO: might be an issue later
        }
      }
    }
  }
  //return Compressed frame
  return frame_comp;
}

void delete_compressed_image(comp_t * image_comp, int num_frames)
{
  for(int i=0;i<num_frames;i++)
  {
    delete image_comp[i].data;
  }

  delete image_comp;
  return;
}


Mat uncompress_frame(comp_t frame)
{

  Mat tmp;

  //int line_buf_size = (int) sizeof(bus_t)*8*ceil(frame.size/frame.data_width);
  //int line_buf_size = (int) ceil(pow((sizeof(bus_t)*8*sqrt(frame.size)/frame.data_width),2));
  int line_buf_size =  400;

  cout<< "line_buffer_size " << line_buf_size << " frame size " << frame.size << endl;
  cout << "Data width: " << frame.data_width << endl;

  pixel_t line_buf[line_buf_size];
  pixel_t mask =  ( 1 << (frame.data_width) ) - 1;

  int line_buf_index = 0;
  int shift_index = 0;

  for(int i=0;i<frame.size;i++) {

    while( shift_index < sizeof(bus_t)*8 )
    {
      if(line_buf_index>=line_buf_size)
      {
        cout << "ERROR (while): " << line_buf_index << " " << i << " " << frame.data_width << endl;
        return tmp;
      }
      line_buf[line_buf_index] = (pixel_t) ( frame.data[i]&(mask<<shift_index) ) >> shift_index ;
      if(shift_index <= (sizeof(bus_t)*8 - frame.data_width))
      {
        line_buf[line_buf_index] = (pixel_t) ((pixel_t_s) line_buf[line_buf_index]) + (pixel_t) frame.avg;
        line_buf_index++;
      }
      shift_index += frame.data_width;
    }
    shift_index = shift_index - sizeof(bus_t)*8;
    if(shift_index&&i<frame.size-1)
    {
      if(line_buf_index>=line_buf_size)
      {
        cout << "ERROR: " << line_buf_index << endl;
        return tmp;
      }
      line_buf[line_buf_index] |=  (pixel_t) ( frame.data[i+1]&(mask>>shift_index) ) << shift_index  ;
      line_buf[line_buf_index] = ( (pixel_t_s) line_buf[line_buf_index]) + (pixel_t) frame.avg;
      line_buf_index++;
    }
  }

  cout << "Line Buffer Size: " << line_buf_index << endl;

  int width_in  = (int) sqrt(line_buf_size);
  int height_in = (int) sqrt(line_buf_size);

  cout << "width: " << width_in << endl;

  //int width_out_max = width_in *

  //find uncompressed size

  //pixel_t img[width*frame.da];

  //for(int i=0;i<)

  return tmp;

}

Mat uncompress_image(comp_t * image_comp, int num_frames)
{
  Mat image;
  Mat tmp;
  for(int i=0;i<num_frames;i++)
  {
    tmp = uncompress_frame(image_comp[i]);
  }

  return image;
}

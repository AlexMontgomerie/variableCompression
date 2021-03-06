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
int find_msb(T data)
{
  int data_size =sizeof(T)*8;
  for(int i=data_size-1;i>=0;i--)
  {
    if(data & (1 << i))
      return i+1;
  }
  return 1;
}

template<typename T>
T sign_extend(T data, int size)
{
  int num_shifts = sizeof(T)*8 - size + 1;
  for(int i=0;i<num_shifts;i++)
  {
    data |= (data&(1<<(size-1))) << (i);   
  }
  return data;
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

comp_t *  get_compressed_image(Mat image, int frame_width, int frame_height);
void      get_frame_pack(comp_t &frame_comp, Mat frame);
double    get_compression_ratio(Mat image, comp_t * image_comp, int num_frames);
void      delete_compressed_image(comp_t * image_comp, int num_frames);
Mat       uncompress_image(comp_t * image_comp, int num_frames);
Mat       uncompress_frame(comp_t &frame);

float get_error(Mat img_original, Mat img)
{
  int num_pixels = img.rows * img.cols;
  int err = 0;

  for(int x=0;x<img.cols;x++) {
    for(int y=0;y<img.cols;y++) {
      pixel_t pixel_original  = img_original.at<pixel_t>(x,y);
      pixel_t pixel           = img.at<pixel_t>(x,y);
      //printf("original: %d, after: %d\n",pixel_original,pixel);
      if(pixel != pixel_original)
        err++; 
    }
  }

  return (float) err/num_pixels;
}

int main()
{

  int frame_width  = 20;
  int frame_height = 20;

  pixel_t tmp_sign = sign_extend<pixel_t>(3, 2);
  printf("test:%d\n", tmp_sign);


  //load in an image
  //Mat img(800, 800, CV_8U, Scalar(255, 250, 30));
  Mat img_tmp = imread("big-test.jpg");
  Mat img;
  cvtColor(img_tmp,img,COLOR_RGB2GRAY);
  if (img.empty())
  {
    cout << "Could not open or find the image" << endl;
    cin.get(); //wait for any key press
    return -1;
  }

  namedWindow("before");
  imshow("before",img);

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

  cout << "Compression Done" << endl;
  
  for(int i=0;i<num_frames;i++) {
    //printf("Pointer: %d\n",image_comp[i].data[0]);
  }

  double comp_ratio = get_compression_ratio(img,image_comp,num_frames);
  printf("compression ratio: %f\n",comp_ratio);

  //Try uncompressing
  Mat img_out = uncompress_image(image_comp, num_frames);

  float err = get_error(img, img_out);
  printf("Error: %f \n", err);
  
  namedWindow("after");
  imshow("after",img_out);
  waitKey(0);
  destroyWindow("after");
  destroyWindow("before");
  

  //delete_compressed_image(image_comp, num_frames);

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
      get_frame_pack(image_comp[array_index],frame);
      //get_frame_pack(frame);
      array_index++;
    }
  }

  //return number of frames
  return image_comp;
}

void get_frame_pack(comp_t &frame_comp, Mat frame)
{

  //number of pixels in frame
  int num_pixels = frame.cols * frame.rows;

  //get mean
  frame_comp.avg = (pixel_t) mean(frame)[0];

  //get max data size
  bus_t max_size = 1;
  for(int i=0;i<frame.rows;i++) {
    for(int j=0;j<frame.cols;j++) {
      int pixel = frame.at<pixel_t>(i,j) - frame_comp.avg ;
      int msb = find_msb<pixel_t>(abs((pixel_t_s) pixel));
      if(msb > max_size)
        max_size = msb;
    }
  }

  //save the width of new data
  max_size = max_size + 1;
  //max_size = 8;
  frame_comp.data_width = max_size;

  //get frame width
  frame_comp.frame_width = frame.cols;

  //save the size of the data
  int num_pixels_comp = (int) ceil((float) num_pixels * max_size / (sizeof(bus_t)*8) );
  frame_comp.size = num_pixels_comp;
  pixel_t mask = ( 1 << (max_size) ) - 1;

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
      pixel_t pixel = (pixel_t) (frame.at<pixel_t>(i,j) - (pixel_t) frame_comp.avg) ;
      pixel &= (pixel_t) mask;

      //add to packet
      frame_comp.data[frame_data_index] |= (pixel << curr_shift);

      //overflow condition
      if( curr_shift >= (sizeof(bus_t)*8 - max_size) )
      {
        //printf("here now\n");
        frame_data_index++;
        if(frame_data_index==num_pixels_comp)
          break;
        curr_shift = curr_shift + max_size - sizeof(bus_t)*8;
        frame_comp.data[frame_data_index] |= (pixel >> (max_size - curr_shift) ); //TODO: might be an issue later
      }
      else  
        curr_shift+=max_size;
    }
  }

 return;
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


Mat uncompress_frame(comp_t &frame)
{

  int line_buf_size =  frame.frame_width*frame.frame_width;
  pixel_t * line_buf = new pixel_t[line_buf_size];
  pixel_t mask = ( ( 1 << (frame.data_width) ) - 1 ) | 1;

  unsigned int line_buf_index = 0;
  unsigned int shift_index = 0;

  
  for(int i=0;i<frame.size;i++) {
    
    // sort out previous data
    if(shift_index)
    {
      line_buf[line_buf_index-1] +=(( (frame.data[i])&(mask>>(frame.data_width - shift_index)) ) << (frame.data_width - shift_index))  ;
      line_buf[line_buf_index-1] = sign_extend<pixel_t>(line_buf[line_buf_index-1], frame.data_width);
      line_buf[line_buf_index-1] = (pixel_t) line_buf[line_buf_index-1] + (pixel_t) frame.avg;
    }

    //get main block of data
    while(shift_index < sizeof(bus_t)*8) 
    {
      if(line_buf_index==line_buf_size)
        break; 
      line_buf[line_buf_index] = (( (frame.data[i])&(mask<<shift_index) ) >> shift_index) ;
      if(shift_index <= (sizeof(bus_t)*8 - frame.data_width))
      {
        line_buf[line_buf_index] = sign_extend<pixel_t>(line_buf[line_buf_index], frame.data_width);
        line_buf[line_buf_index] = (pixel_t) line_buf[line_buf_index] + (pixel_t) frame.avg;
      }
      line_buf_index++;
      shift_index+=frame.data_width;
    }
    shift_index -= sizeof(bus_t)*8;
  }


  int width_in  = frame.frame_width;
  int height_in = frame.frame_width;

  Mat img(width_in,height_in,CV_8U,line_buf);
  return img;
}

Mat uncompress_image(comp_t * image_comp, int num_frames)
{
  Mat image;
  Mat h_buffer;

  int width = (int) sqrt(num_frames);
  int height = width;

  int frame_index = 0;
  for(int i=0;i<height;i++) {
    for(int j=0;j<width;j++) {
      Mat tmp = uncompress_frame(image_comp[frame_index]);
      frame_index++;

      //first pixel
      if(i==0&&j==0)
        h_buffer = tmp;

      //first_row
      if(i==0&&j!=0)
        hconcat(h_buffer,tmp,h_buffer);

      //first row added
      if(i==1&&j==0)
      {
        image = h_buffer;
        h_buffer = tmp;
      } 

      if(i==1&&j!=0)
        hconcat(h_buffer,tmp,h_buffer);

      //new row
      if(i>1&&j==0)
      {
        vconcat(image,h_buffer,image);
        h_buffer = tmp;
      }

      //regular appending to row
      if(i>1&&j!=0)
        hconcat(h_buffer,tmp,h_buffer);
    }
  }

  //add last row
  vconcat(image,h_buffer,image);
  
  cout << "image width: " <<image.cols << " Image Height: "<<image.rows <<endl;
  
  return image;
}

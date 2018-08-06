#include "get_dataset.hpp"
#include "var_comp.hpp"

void read_batch_cifar(string filename, vector<Mat> &vec, Mat &label)
{
  ifstream file (filename, ios::binary);
  if(file.is_open())
  {
    int num_images = 10000;
    int n_rows = 32;
    int n_cols = 32;

    for(int i=0;i<num_images;++i)
    {
      unsigned char tplabel = 0;
      vector<Mat> channels;
      Mat fin_img = Mat::zeros(n_rows,n_cols,CV_8UC3);
      for(int ch=0;ch<3;++ch)
      {
        Mat tp = Mat::zeros(n_rows,n_cols,CV_8UC1);
        for(int r=0;r<n_rows;++r)
        {
          for(int c=0;c<n_cols;++c)
          {
            unsigned char temp = 0;
            file.read((char *) &temp,sizeof(temp));
            tp.at<uchar>(r,c) = (int) temp;
          }
        }
        channels.push_back(tp);
      }
      merge(channels,fin_img);
      vec.push_back(fin_img);
      label.ATD(0,i) = (double)tplabel;
    }
  }
}

Mat get_fixed_size_caltech(Mat &in, int width, int height)
{

  Mat output;

  if(width > in.cols || height > in.rows)
  {

    int border_right;
    int border_top;
    int crop_right;
    int crop_top;

    //width
    if(width>in.cols)
    {
      border_right  = width - in.cols;
      crop_right    = in.cols;
    }
    else
    {
      border_right  = 0;
      crop_right    = width;
    }

    //height
    if(height>in.rows)
    {
      border_top  = height - in.rows;
      crop_top    = in.rows;
    }
    else
    {
      border_top  = 0;
      crop_top    = height;
    }
    
    Rect crop(0,0,crop_right,crop_top);
    
    copyMakeBorder(in(crop), output,border_top,0,0,border_right,BORDER_REPLICATE,Scalar(0,0,0));
  
  }
  else
  {
    Rect crop(0,0,width,height);
    output = in(crop);
  }
  return output;
}

void read_caltech(vector<Mat> &vec)
{
  
  for(int i=0;i<801;i++)
  {
    //create filename
    string filename = (string) CALTECH_DIR + "image_0" + to_string(i) + ".jpg";
    
    ifstream ifile(filename.c_str());
    if(ifile)
    {
      Mat tmp = imread(filename);
      tmp = get_fixed_size_caltech(tmp, CALTECH_WIDTH, CALTECH_HEIGHT);
      string ty =  type2str( tmp.type() );
      //printf("Matrix: %s %dx%d \n", ty.c_str(), tmp.cols, tmp.rows );
      vec.push_back(tmp);
    }
  }
}

#include "var_comp.hpp"
#include "get_dataset.hpp"

#define FRAME_WIDTH   20
#define FRAME_HEIGHT  20 

int main()
{

  int frame_width  = FRAME_WIDTH;
  int frame_height = FRAME_HEIGHT;

  vector<Mat> images;
  Mat labels = Mat::zeros(1, 10000, CV_64FC1);

  //get dataset
  //read_batch_cifar(CIFAR_BATCH_1, images, labels);
  read_caltech(images);

  //iterate over images
  for(int i=0;i<images.size();i++)
  {
    vector<Mat> channels(3);
    split(images[i],channels);
    
    float cr=0.0;  

    for(int c=0;c<3;c++)
    {
      int num_frames = get_num_frames(channels[c],frame_width,frame_height);
      comp_t * comp_tmp;

      //Mat tmp = channels[i];
      //tmp.convertTo(tmp, CV_8U);
      
      //check image
      /*
      namedWindow("test");
      imshow("test",tmp);
      waitKey(0);
      destroyWindow("test");     
      */
  
      //compress image
      comp_tmp = get_compressed_image(channels[c], frame_width, frame_height);
      
      //get compression ratio
      cr += get_compression_ratio(channels[c], comp_tmp, num_frames);
    }
    
    printf("Average comp ratio: %f\n", (float) cr/3.0);

  }

  //completed script
  return 0;

}


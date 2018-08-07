#include "var_comp.hpp"
#include "get_dataset.hpp"

#define FRAME_WIDTH   10
#define FRAME_HEIGHT  10 

int frame_sizes[6] = {5,10,20,25,50,100};

int main()
{

  vector<Mat> images;
  Mat labels = Mat::zeros(1, 10000, CV_64FC1);

  //get dataset
  //read_batch_cifar(CIFAR_BATCH_1, images, labels);
  read_caltech(images);

  float cr = 0.0;

  for(int j=0;j<6;j++)  
  {

    int frame_width  = frame_sizes[j];
    int frame_height = frame_sizes[j];


    //iterate over images
    for(int i=0;i<images.size();i++)
    {
      vector<Mat> channels(3);
      split(images[i],channels);
      
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
      
      //printf("Average comp ratio: %f\n", (float) cr/3.0);

    }

    printf( "Average comp ratio: %f, (%d,%d)\n", (float) cr/( 3.0*images.size() ) , frame_width, frame_height);
    cr = 0.0;
  }
  //completed script
  return 0;
}


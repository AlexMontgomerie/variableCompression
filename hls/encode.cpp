#include "encode.hpp"

void encode(
    hls::stream<data_t> &in, 
    hls::stream<stream_t> &out
  )
{

  //frame buffer
  hls::stream<data_t> frame_buf;
//TODO: pragma

  //number of pixels in frame
  int num_pixels = FRAME_WIDTH * FRAME_HEIGHT;

  //get max data size (loop 1)
  data_t max_size = 1;
  
  bus_t avg = 0;

  for(int i=0;i<FRAME_HEIGHT;i++) {
    for(int j=0;j<FRAME_WIDTH;j++) {
      int pixel; 
      in >> pixel;

      //store in buffer
      frame_buffer << pixel;

      //increment sum
      avg+=pixel;      

      //find msb
      int msb = find_msb<pixel_t>(abs((pixel_t_s) pixel)); //TODO: implement in hw
      if(msb > max_size)
        max_size = msb;
    }
  }

  avg = (bus_t) (avg/(FRAME_WIDTH*FRAME_HEIGHT));
  //send the average (head of packet)

  out << avg;

  //save the width of new data
  max_size = max_size + 1;
  
  out << (bus_t) max_size;

  //save the size of the data
  bus_t num_pixels_comp = (int) ceil((float) num_pixels * max_size / (sizeof(bus_t)*8) );
 
  //send size
  out << num_pixels_comp;
 
  data_t mask = ( 1 << (max_size) ) - 1;

  int frame_data_index = 0;
  int curr_shift = 0;

  bus_t out_tmp = 0;

  for(int i=0;i<FRAME_HEIGHT;i++) {
    for(int j=0;j<FRAME_WIDTH;j++) {
      pixel_t pixel;
      frame_buffer >> pixel;
      pixel = ( (data_t) (pixel - avg) ) & ( (data_t) mask );

      //add to packet
      out_tmp |= (pixel << curr_shift);

      //overflow condition
      if( curr_shift >= (sizeof(bus_t)*8 - max_size) )
      {
        //sending out packet
        frame_data_index++;
        out << out_tmp;
        out_tmp = 0;

        if(frame_data_index==num_pixels_comp)
          break;
        
        curr_shift = curr_shift + max_size - sizeof(bus_t)*8;
        out_tmp |= (pixel >> (max_size - curr_shift) ); //TODO: might be an issue later
      }
      else  
        curr_shift+=max_size;
    }
  }
}

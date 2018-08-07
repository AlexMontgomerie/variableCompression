

void decode(
    hls::stream<stream_t> &in,
    hls::stream<data_t> &out
  )
{

//TODO: set hls stream pragma, and their depth

  bus_t data_width;
  bus_t avg;
  bus_t size;

  in >> data_width;
  in >> avg;
  in >> size;

  int line_buf_size =  FRAME_HEIGHT*FRAME_WIDTH;
  
  //pixel_t * line_buf = new pixel_t[line_buf_size];
  data_t mask = ( ( 1 << (data_width) ) - 1 ) | 1;

  unsigned int line_buf_index = 0;
  unsigned int shift_index = 0;
  
  data_t out_tmp;

  for(int i=0;i<size;i++) {
//TODO: maybe pipeline?


    bus_t frame_data;
    in >> frame_data;
    
    // sort out previous data
    if(shift_index)
    {
      out_tmp += (( (frame_data)&(mask>>(data_width - shift_index)) ) << (data_width - shift_index))  ;
      out_tmp =  sign_extend<data_t>(out_tmp, data_width); //TODO: implement in hw
      out_tmp =  (data_t) out_tmp + (data_t) avg;
      out << out_tmp;
    }

    //get main block of data
    while(shift_index < sizeof(bus_t)*8) 
    {
      if(line_buf_index==line_buf_size)
        break; 
      out_tmp = (( (frame_data)&(mask<<shift_index) ) >> shift_index) ;
      if(shift_index <= (sizeof(bus_t)*8 - data_width))
      {
        out_tmp = sign_extend<pixel_t>(out_tmp, data_width);
        out_tmp = (data_t) out_tmp + (data_t) avg;
        out << out_tmp;
      }
      line_buf_index++;
      shift_index+=data_width;
    }
    shift_index -= sizeof(bus_t)*8;
  }
}

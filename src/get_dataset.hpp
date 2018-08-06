#ifndef GET_DATASET_HPP_
#define GET_DATASET_HPP_

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <fstream>
#include <iostream>

#define ATD at<double>

///////////////////
// CIFAR DATASET //
///////////////////

#define CIFAR_ROWS 32
#define CIFAR_COLS 32

#define CIFAR_BATCH_1 "../datasets/cifar-10-batches-bin/data_batch_1.bin"
#define CIFAR_BATCH_2 "../datasets/cifar-10-batches-bin/data_batch_2.bin"
#define CIFAR_BATCH_3 "../datasets/cifar-10-batches-bin/data_batch_3.bin"
#define CIFAR_BATCH_4 "../datasets/cifar-10-batches-bin/data_batch_4.bin"
#define CIFAR_BATCH_5 "../datasets/cifar-10-batches-bin/data_batch_5.bin"

/////////////////////////
// 101 CALTECH DATASET //
/////////////////////////

#define CALTECH_WIDTH  300
#define CALTECH_HEIGHT 200

#define CALTECH_DIR "../datasets/101_ObjectCategories/"

using namespace cv;
using namespace std;

void read_batch_cifar(string filename, vector<Mat> &vec, Mat &label);
void read_caltech(vector<Mat> &vec);

#endif

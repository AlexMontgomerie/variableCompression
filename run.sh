g++ --std=c++11 -lstdc++ -g compressor.cpp -o test `pkg-config --cflags --libs opencv`
./test

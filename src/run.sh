g++ --std=c++11 -lstdc++ -Wall -g get_dataset.cpp var_comp.cpp var_comp_tb.cpp -o test `pkg-config --cflags --libs opencv`
./test

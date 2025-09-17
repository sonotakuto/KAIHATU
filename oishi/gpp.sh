export DISPLAY=10.19.133.125:0
g++ -o error_test error_test.cpp `pkg-config --cflags --libs gtk+-3.0`
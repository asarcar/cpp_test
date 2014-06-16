# Build the executable
> cd $PROJ_DIR/build
> cmake VERBOSE=1 .. -DCMAKE_BUILD_TYPE=Debug
> make

# Run the executable
> cd $PROJ_DIR/test_ip_op
> ../bin/mc_test_d --log_dir="." --num_vals=100 --val_size=200
> ../bin/mc_test_d --log_dir="." --num_vals=100 --val_size=200

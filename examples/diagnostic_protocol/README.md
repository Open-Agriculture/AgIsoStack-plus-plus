cd ISO11783-CAN-Stack
cmake -S . -B build && cmake --build build && cmake --install build --prefix install
cd examples/diagnostic_protocol
cmake -S . -B build -DCMAKE_PREFIX_PATH=../../install && cmake --build build
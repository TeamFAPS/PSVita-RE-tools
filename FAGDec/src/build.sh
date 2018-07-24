cd kplugin
rm -rf build
mkdir build
cd build
cmake ..
make install
cd ../../uplugin/
rm -rf build
mkdir build
cd build
cmake ..
make install
cd ../../app/
rm -rf build
mkdir build
cd build
cmake ..
make all
cd ../..
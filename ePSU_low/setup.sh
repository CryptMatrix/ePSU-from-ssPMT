mkdir -p thirdparty
cd thirdparty

# Build volepsi
git clone https://github.com/Th0masAndy/volepsi.git
cd volepsi
sed -i '157s/co_await(generateTriple(1 << 20, 2, chl));/co_await(generateTriple(1 << 22, 2, chl));/' volePSI/GMW/Gmw.cpp
python3 build.py --install=../out/install -DVOLE_PSI_ENABLE_BOOST=ON -DVOLE_PSI_ENABLE_GMW=ON -DVOLE_PSI_ENABLE_CPSI=OFF -DVOLE_PSI_ENABLE_OPPRF=OFF
cp out/build/linux/volePSI/config.h ../out/install/include/volePSI


cd ../..
mkdir build
cd build
cmake ..
make -j



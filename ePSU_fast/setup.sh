mkdir -p thirdparty
cd thirdparty

# Build secure-join
git clone https://github.com/Th0masAndy/secure-join
cd secure-join
git checkout 4a23526f4b3a8432f7fb12d54b9865e95faedcf4
python3 build.py --install=../out/install -DFETCH_ALL=ON SECUREJOIN_ENABLE_BOOST=ON -D SODIUM_MONTGOMERY=false -D ENABLE_BITPOLYMUL=false 
cd ..

# Build volepsi
git clone https://github.com/Th0masAndy/volepsi.git
cd volepsi
sed -i '157s/co_await(generateTriple(1 << 20, 2, chl));/co_await(generateTriple(1 << 18, 2, chl));/' volePSI/GMW/Gmw.cpp
python3 build.py --install=../out/install  --system -DVOLE_PSI_ENABLE_BOOST=true -DVOLE_PSI_ENABLE_BITPOLYMUL=false -DVOLE_PSI_SODIUM_MONTGOMERY=false -DCMAKE_PREFIX_PATH=/usr/local/ 
cp ./out/build/linux/volePSI/config.h ../out/install/include/volePSI
cd ../..


# Build ePSU
mkdir -p build
cd build
cmake ..
make -j
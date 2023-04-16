export TOP=$(pwd)
export STATE_FILE=$TOP/state
export NPROC=$(nproc)

echo -n "" > $STATE_FILE

# Скачиваем сорцы gcc-12.2.0
echo "download gcc" >> $STATE_FILE
wget https://mirrorservice.org/sites/sourceware.org/pub/gcc/releases/gcc-12.2.0/gcc-12.2.0.tar.gz
tar -xf gcc-12.2.0.tar.gz
mv gcc-12.2.0 src
echo "  ok" >> $STATE_FILE

# Применяем патчи и скачиываем либы
echo "apply patches" >> $STATE_FILE
cd src
wget https://raw.githubusercontent.com/Nechda/fsort/master/patched-gcc/gcc-12.p
patch -p1 < gcc-12.p
./contrib/download_prerequisites
cd $TOP
echo "  ok" >> $STATE_FILE

# Собираем ванильный gcc
echo "build vanila gcc" >> $STATE_FILE
mkdir -p gcc-build
mkdir -p gcc-install
export INSTALL_DIR=$(pwd)/gcc-install

cd gcc-build
../src/configure --prefix=$INSTALL_DIR --enable-languages=c,c++,lto --disable-werror --enable-gold --disable-multilib --disable-bootstrap
echo "  configure -> ok" >> $STATE_FILE
make all -j $NPROC
echo "  make all -> ok" >> $STATE_FILE
make install
echo "  make install -> ok" >> $STATE_FILE
cd $TOP

# Меняем компилятор
echo "change PATH" >> $STATE_FILE
export PATH="$TOP/gcc-install/bin:$PATH"
g++ --version >> $STATE_FILE
echo "  ok" >> $STATE_FILE

# Еще раз собираем gcc, но уже с lto
echo "build gcc with lto" >> $STATE_FILE
mkdir -p gcc-build-lto
mkdir -p gcc-install-lto
export INSTALL_DIR=$(pwd)/gcc-install-lto

cd gcc-build-lto
../src/configure --prefix=$INSTALL_DIR --enable-languages=c,c++,lto --disable-werror --enable-gold --disable-multilib --disable-bootstrap
echo "  configure -> ok" >> $STATE_FILE
make CFLAGS="-O2 -flto" CXXFLAGS="-O2 -flto" all -j $NPROC
echo "  make all -> ok" >> $STATE_FILE
make install
echo "  make install -> ok" >> $STATE_FILE
cd $TOP

# Копируем репозиторий с утилитой
git clone https://github.com/Nechda/fsort.git f-src

# Собираем плагин
echo "Build plugin" >> $STATE_FILE
cd f-src/reorder-plugin
TARGET_GCC=g++ make && mv freorder-ipa.so $TOP
echo "  ok" >> $STATE_FILE
cd $TOP

# Собираем утилиту
echo "Build fsort" >> $STATE_FILE
mkdir -p fsort && cd fsort
cmake -GNinja ../f-src
ninja fsort
echo "  ok" >> $STATE_FILE

# Собираем профиль
echo "Collect profile & gen optimal order" >> $STATE_FILE
export CC1PLUS=$TOP/gcc-install-lto/libexec/gcc/x86_64-pc-linux-gnu/12.2.0/cc1plus
export EXEC_FILTER=$(basename $CC1PLUS)
export BNCH_CMD="$TOP/gcc-install-lto/bin/g++ -w $TOP/f-src/benchmarks/tramp3d-v4.cpp -o /dev/null"
export ORDER=$(pwd)/sorted.out
echo "  binary file = $CC1PLUS" >> $STATE_FILE
echo "  exec filter = $EXEC_FILTER" >> $STATE_FILE
echo "  benchmark command = $BNCH_CMD" >> $STATE_FILE
./fsort --binary=$CC1PLUS --output=sorted.out --runs=4 --delta=1 --exec-file="$EXEC_FILTER" --command="$BNCH_CMD"
echo "  output order = $ORDER" >> $STATE_FILE
echo "  ok" >> $STATE_FILE
cd $TOP

# Перелинкуем cc1plus
echo "Relink cc1plus file with profile" >> $STATE_FILE
export PLUGIN_SO=$TOP/freorder-ipa.so
export GCC_BUILD_DIR=$TOP/gcc-build-lto
cd $GCC_BUILD_DIR/gcc
echo "  relink cc1plus" >> $STATE_FILE
bash -ex $TOP/f-src/scripts/build_cc1plus.sh
echo "    ok" >> $STATE_FILE
echo "  replace original cc1plus" >> $STATE_FILE
mv $CC1PLUS $TOP/cc1plus.vanila
cp cc1plus $TOP/cc1plus.reorder
mv cc1plus $CC1PLUS
echo "    ok" >> $STATE_FILE

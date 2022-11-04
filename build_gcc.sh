#! /usr/bin/bash -xe
export PROJECT_TOP=$(pwd)
[ -d ~/tmp ] || (echo "Error: please create ~/tmp dir" && false)
export TMPDIR=~/tmp
N_JOBS=$(nproc)

# ================================================
# 1. Сборка ванильного gcc
#   1.1 Клонирование оригинального репозитория gcc
#   1.2 Применение патча Мартина
#   1.3 Установка необходимых пакетов
#   1.4 Компиляция ванильной версии
#   1.5 Обновление переменной окружения PATH
# ================================================
cd $PROJECT_TOP/patched-gcc
[ -d gcc-build ] || mkdir -p gcc-build
[ -d gcc-install ] || mkdir -p gcc-install

if [[! -e gcc-vanilla ]]; then
    # Клонирование оригинального репозитория gcc
    git clone git://gcc.gnu.org/git/gcc.git gcc-vanilla
    # Применение патча Мартина
    cd gcc-vanilla
    git reset --hard 13c83c4cc679ad5383ed57f359e53e8d518b7842
    patch -p 1 < ../freorder-ipa.patch

    # Установка необходимых пакетов
    ./contrib/download_prerequisites
    cd ..
fi

# Компиляция ванильной версии
export INSTALL_DIR=$(pwd)/gcc-install
cd gcc-build
../gcc-vanilla/configure --prefix=$INSTALL_DIR \
--enable-languages=c,c++,lto --disable-werror --enable-gold \
--disable-multilib --disable-bootstrap
make -j $N_JOBS
make install
cd ..

# Обновление переменной окружения PATH
export PATH="$(pwd)/gcc-install/bin:$PATH"


# ================================================
# 2. Компилирование планига для gcc ver 9.4.0
# ================================================
cd $PROJECT_TOP/reorder-plugin
export TARGET_GCC="$INSTALL_DIR/bin/gcc"
make clean && make all
export PLUGIN_SO=$(pwd)/freorder-ipa.so


# ================================================
# 3. Компиляция lto версии gcc
#   3.1 Конфигурирование
#   3.2 Принудительное выставление флагов компиляции
#   3.3 Сборка и установка gcc
#   3.4 Сохранение неоптимизированной версии
# ================================================
cd $PROJECT_TOP/patched-gcc
[ -d gcc-build-lto ] || mkdir -p gcc-build-lto
[ -d gcc-install-lto ] || mkdir -p gcc-install-lto

# Конфигурирование
export INSTALL_DIR=$(pwd)/gcc-install-lto
cd gcc-build-lto
../gcc-vanilla/configure --prefix=$INSTALL_DIR \
--enable-languages=c,c++,lto --disable-werror --enable-gold \
--disable-multilib --disable-bootstrap
# Принудительное выставление флагов компиляции
patch < ../lto-gcc.patch
# Сборка и установка gcc
make -j $N_JOBS
make install
# Сохранение неоптимизированной версии
cp $INSTALL_DIR/libexec/gcc/x86_64-pc-linux-gnu/9.4.0/cc1plus \
   $PROJECT_TOP/patched-gcc/cc1plus.lto.orig


# ================================================
# 4. Сбор профиля
#   4.1 Компиляция утилиты fsort
#   4.2 Генерация оптимального порядка
# ================================================
cd $PROJECT_TOP && mkdir -p fsort_utility && cd fsort_utility

# Компиляция утилиты fsort
cmake -GNinja .. && ninja

# Генерация оптимального порядка
CC1PLUS="$INSTALL_DIR/libexec/gcc/x86_64-pc-linux-gnu/9.4.0/cc1plus"
./fsort --binary=$CC1PLUS --output=sorted.out --runs=64 --delta=1 \
--command="$INSTALL_DIR/bin/g++ -w $PROJECT_TOP/benchmarks/tramp3d-v4.cpp -o /dev/null"

# ================================================
# 5. Пересборка gcc с учетом профиля
#   5.1 Перелинковка cc1plus с учетом профля
#   5.2 Установка
#   5.3 Сохранение оптимизированной версии
# ================================================
# Перелинковка cc1plus с учетом профля
cd $PROJECT_TOP/patched-gcc/gcc-build-lto
export GCC_BUILD_DIR=$(pwd)
export ORDER=$PROJECT_TOP/fsort_utility/sorted.out
bash -ex $PROJECT_TOP/build_cc1plus.sh
# Установка
make install
# Сохранение оптимизированной версии
cp $INSTALL_DIR/libexec/gcc/x86_64-pc-linux-gnu/9.4.0/cc1plus \
   $PROJECT_TOP/patched-gcc/cc1plus.lto.sorted
cd gcc
g++ -no-pie -g -O2 -flto -freorder-functions -fplugin=~/gcc-patched/reorder_plugin/c3-ipa.so \
 -DIN_GCC -fno-exceptions -fno-rtti -fasynchronous-unwind-tables -W -Wall -Wno-narrowing \
 -Wwrite-strings -Wcast-qual -Wmissing-format-attribute -Woverloaded-virtual -pedantic \
 -Wno-long-long -Wno-variadic-macros -Wno-overlength-strings -DHAVE_CONFIG_H -static-libstdc++ \
 -static-libgcc -o cc1plus \
 cp/cp-lang.o c-family/stub-objc.o cp/call.o cp/class.o cp/constexpr.o cp/constraint.o cp/cp-gimplify.o \
 cp/cp-objcp-common.o cp/cp-ubsan.o cp/cvt.o cp/cxx-pretty-print.o cp/decl.o cp/decl2.o cp/dump.o cp/error.o \
 cp/except.o cp/expr.o cp/friend.o cp/init.o cp/lambda.o cp/lex.o cp/logic.o cp/mangle.o cp/method.o cp/name-lookup.o \
 cp/optimize.o cp/parser.o cp/pt.o cp/ptree.o cp/repo.o cp/rtti.o cp/search.o cp/semantics.o cp/tree.o cp/typeck.o \
 cp/typeck2.o cp/vtable-class-hierarchy.o attribs.o incpath.o c-family/c-common.o c-family/c-cppbuiltin.o \
 c-family/c-dump.o c-family/c-format.o c-family/c-gimplify.o c-family/c-indentation.o c-family/c-lex.o \
 c-family/c-omp.o c-family/c-opts.o c-family/c-pch.o c-family/c-ppoutput.o c-family/c-pragma.o c-family/c-pretty-print.o \
 c-family/c-semantics.o c-family/c-ada-spec.o c-family/c-ubsan.o c-family/known-headers.o c-family/c-attribs.o \
 c-family/c-warn.o c-family/c-spellcheck.o i386-c.o glibc-c.o cc1plus-checksum.o libbackend.a main.o libcommon-target.a \
 libcommon.a ../libcpp/libcpp.a ../libdecnumber/libdecnumber.a libcommon.a ../libcpp/libcpp.a ../libbacktrace/.libs/libbacktrace.a \
 ../libiberty/libiberty.a ../libdecnumber/libdecnumber.a -L~/gcc-patched/gcc-build/./isl/.libs -lisl \
 -L~/gcc-patched/gcc-build/./gmp/.libs -L~/gcc-patched/gcc-build/./mpfr/src/.libs \
 -L~/gcc-patched/gcc-build/./mpc/src/.libs -lmpc -lmpfr -lgmp -rdynamic -ldl -L./../zlib -lz

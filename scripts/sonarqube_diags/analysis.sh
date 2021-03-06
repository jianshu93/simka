# This script launches several diagnostics, whose results will be uploaded to the
# SonarQube server.
#!/bin/bash

echo_stderr() { echo "$@" 1>&2; }

echo_stderr "===> Launching analysis script (to prepare SonarQube diagnostics)..."

####################################################################################################
# CppCheck and RATS analysis
####################################################################################################
echo_stderr "===> Launching CppCheck analysis..."
cppcheck -j2 -f --enable=all --suppress=missingIncludeSystem --xml-version=2 src/ &> simka-cppcheck.xml

echo_stderr "===> Launching RATS analysis..."
rats -w 3 --xml src > simka-rats.xml

####################################################################################################
# Compile the code
####################################################################################################
mkdir build
cd build

# compilation options
echo_stderr "===> Compilation options"
CFLAGS="--coverage \
	-fPIC -fdiagnostics-show-option \
	-Wall -Wunused-parameter -Wundef -Wno-long-long \
	-Wsign-compare -Wmissing-prototypes -Wstrict-prototypes -Wcomment \
	-pedantic -g"
LDFLAGS="--coverage"
echo_stderr "CFLAGS: $CFLAGS"
echo_stderr "LDFLAGS: $LDFLAGS"

# launch cmake
echo_stderr "===> Launching cmake with scan-build..."

scan-build -v -plist --intercept-first --analyze-headers -o analyzer_reports \
      cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_CXX_FLAGS="$CFLAGS" -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS" \
      -DCMAKE_BUILD_TYPE=DEBUG \
      -DCMAKE_CXX_OUTPUT_EXTENSION_REPLACE=ON &> simka-scan-build-cmake.log

# launch make
echo_stderr "===> Launching make with scan-build..." 
time scan-build -v -plist --intercept-first --analyze-headers -o analyzer_reports \
    make -j 4 &> simka-scan-build-make.log
 #  make -j 4

[[ -x "bin/simka" ]] || { echo "Error, simka executable not generated"; exit 111; }

mv simka-scan-build-cmake.log  ..
mv simka-scan-build-make.log  ..
mv analyzer_reports ..

####################################################################################################
# Clang-tidy analysis
####################################################################################################
echo_stderr "===> Launching run-clang-tidy..."
test -f compile_commands.json || echo "Warning, compilation database missing"
run-clang-tidy-3.8.py -checks='*'  -p . -j4 > simka-clang-tidy-report.log

mv simka-clang-tidy-report.log ..
mv compile_commands.json ..

####################################################################################################
# Coverage analysis
####################################################################################################

echo_stderr "===> Launching lcov (initial)..."

# run initial/baseline lcov
lcov --capture --initial --directory . --output-file simka_coverage_base.info

# run tests
echo_stderr "===> Launching a simple test run..."
../example/simple_test.sh

# run lcov again after tests complete
echo_stderr "===> Launching lcov (after test completion)..."
lcov --capture --directory . --output-file simka_coverage_test.info

# combine lcov tracefiles
lcov --add-tracefile simka_coverage_base.info \
    --add-tracefile simka_coverage_test.info \
    --output-file simka_coverage_total.info

# extract useful data
lcov --extract simka_coverage_total.info '/home/gitlab/simka/src/*' \
    --output-file simka_coverage_total_filtered.info

# generate the html lcov page that can be published
echo_stderr "===> Generating coverage html pages..."
genhtml -o coverage-html simka_coverage_total_filtered.info
mv coverage-html ..

# convert the lcov report to an xml format convertible with SonarQube
echo_stderr "===> Generating coverage report for SonarQube..."
lcov_cobertura.py simka_coverage_total_filtered.info --output gcov.xml
mv gcov.xml ..

echo_stderr "===> Done..."

bazel build --jobs 4  --local_cpu_resources=4 --cxxopt=-std=c++14 //src:psiServer
bazel build --cxxopt=-std=c++14 //src:psiClient
rm -rf output/
mkdir output
cp bazel-bin/src/psiClient output/
cp bazel-bin/src/psiServer output/
cp -r config/ output/

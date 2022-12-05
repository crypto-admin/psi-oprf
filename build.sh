bazel build //src:psiServer
bazel build //src:psiClient
rm -rf output/
mkdir output
cp bazel-bin/src/psiClient output/
cp bazel-bin/src/psiServer output/
cp -r config/ output/
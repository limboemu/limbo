# for debugging
#export USE_OPTIMIZATION=false
#export NDK_DEBUG=1

export BUILD_HOST=arm64-v8a
export BUILD_GUEST=x86_64-softmmu
make limbo

#!/bin/sh

while [ "$#" -gt 0 ]; do
    case "$1" in
        --macos-deployment-target=*)
            DEPLOYMENT_TARGET="${1#--macos-deployment-target=}"
            ;;
        --macos-deployment-target)
            DEPLOYMENT_TARGET="$2"
            shift
            ;;
    esac
    shift
done

if [ -z "$DEPLOYMENT_TARGET" ] || [ "$DEPLOYMENT_TARGET" = "default" ]; then
    DEPLOYMENT_TARGET=14.0
fi

cd modules/sentry-native
cmake -B build -DSENTRY_BUILD_SHARED_LIBS=OFF -DSENTRY_BACKEND=crashpad -DSENTRY_SDK_NAME="sentry.native.godot" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET"
cmake --build build --target sentry --parallel --config RelWithDebInfo
cmake --build build --target crashpad_handler --parallel --config RelWithDebInfo
cmake --install build --prefix install --config RelWithDebInfo
cd ../..

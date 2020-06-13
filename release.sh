#!/bin/bash -e

MQTTBRIDGE_VERSION=0.1.0
PACKAGE_VERSION=0
VERSION="${MQTTBRIDGE_VERSION}-${PACKAGE_VERSION}"

die() {
	echo "$*"
	exit 1
}

MACH=$(uname -m)
[ "$MACH" = x86_64 ] && ARCH=amd64
[ "$MACH" = armv7l ] && ARCH=arm32v7
[ -z "$ARCH" ] && die "unknown machine: $MACH"

IMAGE="thzpub/knxd:${ARCH}-${VERSION}"
docker build -t "$IMAGE" .
docker push "$IMAGE"

echo "Pushed $IMAGE"

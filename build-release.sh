#!/bin/bash -e

MQTTBRIDGE_VERSION=0.3.0
PACKAGE_VERSION=1

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
echo "Built $IMAGE"

if [ "$1" = "--push" ]; then
	docker push "$IMAGE"
	echo "Pushed $IMAGE"
fi

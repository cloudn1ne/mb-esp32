#!/bin/bash
#
# Build the mb-esp32-web app and copy it into the ESP32 LittleFS "www" folder.
#
#   ./deploy.sh            # build + copy to ../data/www
#   ./deploy.sh --flash    # also upload filesystem to the ESP32 (pio run -t uploadfs)
#
set -e
cd "$(dirname "$0")"

echo "==> Building web app"
npm run build

echo "==> Copying to ../data/www"
rm -rf ../data/www
mkdir -p ../data/www
cp -r dist/* ../data/www/

echo "==> Done. data/www now contains:"
ls -la ../data/www

if [ "$1" = "--flash" ]; then
    echo "==> Uploading filesystem to ESP32"
    cd ..
    pio run -t uploadfs
fi
#!/bin/sh

if [ ! -f makefsdata ]; then
    # Doing this outside cmake as we don't want it cross-compiled but for host
    echo Compiling makefsdata
    gcc -o makefsdata -Ipico-sdk/lib/lwip/src/include -Ipico-sdk/lib/lwip/contrib/ports/unix/port/include -I. -DMAKEFS_SUPPORT_DEFLATE=1 -DMAKEFS_SUPPORT_DEFLATE_ZLIB=1 -DHTTPD_ADDITIONAL_CONTENT_TYPES="{\"svg\", HTTP_HDR_SVG }" pico-sdk/lib/lwip/src/apps/http/makefsdata/makefsdata.c -lz
fi

# Build a new version of the web interface
npm run --prefix web-interface/ build

# Copy the built files into fs
rm -rf ./fs
cp -R web-interface/build ./fs

# delete the map files
find ./fs -name "*.map" -delete -print
find ./fs -name "*.txt" -delete -print
# overwrite the debug api data with the template files
rename -f -v 's/.template$//' ./fs/api/*.template
rename -f -v 's/.template$//' ./fs/api/**/*.template

echo Regenerating fsdata.c
./makefsdata -defl:9 -svr:picow
echo Done
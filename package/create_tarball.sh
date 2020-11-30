#! /bin/bash

PACKAGE_DIRS_AND_FILES="data src"
PACKAGE_DIRS_AND_FILES="$PACKAGE_DIRS_AND_FILES Makefile README SConstruct LICENSE"

PACKAGE_NAME="lbm_opencl_fs_`date +%Y_%m_%d`"
PACKAGE_DIR="$PACKAGE_NAME"
PACKAGE_TARBALL="$PACKAGE_NAME.tar.bz2"

echo "Creating package $PACKAGE_NAME"
rm -f -r "$PACKAGE_DIR"
mkdir "$PACKAGE_DIR"

echo " + copying files"
for file in $PACKAGE_DIRS_AND_FILES; do
	cp -r "../$file" "$PACKAGE_DIR"
done

echo " + removing svn information"
# remove svn from package directory
cd "$PACKAGE_DIR" && { find ./ -name ".svn" | xargs rm -Rf; } && cd ..

echo " + creating tarball $PACKAGE_TARBALL"
rm -f "$PACKAGE_TARBALL"
tar cjf "$PACKAGE_TARBALL" "$PACKAGE_DIR"

echo " + cleaning up"
rm -r "$PACKAGE_DIR"

echo "Done!"

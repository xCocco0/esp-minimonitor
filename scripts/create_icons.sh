#!/bin/bash

USAGE="$(basename $0) <source_dir> <destination_dir> <new_size>"

### PARSE INPUT ###

if [ $1 ] && [ -d "$1" ]
then
	SRC_DIR=$1
else
	>&2 echo $USAGE
	exit 1
fi

if [ $2 ] && [ -d "$(dirname $2)" ]
then
	DST_DIR=$2
else
	>&2 echo $USAGE
	exit 1
fi

if [ $3 ]
then
	SIZE="$3"
else
	>&2 echo $USAGE
	exit 1
fi

### FIND LVGL CONVERTER SCRIPT ###

LV_CONV8=$(find $LV_IMG_CONV_DIR -maxdepth 1 -name "lv_img_conv.js" | head -n1)
[ -z "$LV_CONV8" ] && LV_CONV8=$(find . -maxdepth 1 -name "lv_img_conv.js" | head -n1)
[ -z "$LV_CONV8" ] && LV_CONV8=$(find ~ -maxdepth 2 -name "lv_img_conv.js" | head -n1)
if ! [ -f "$LV_CONV8" ]
then
	>&2 echo "Could not find lv_img_conf.js"
	>&2 echo "See https://github.com/lvgl/lv_img_conv"
	exit 1
fi

LV_CONV9=$(find $LV_IMG_CONV_DIR -maxdepth 1 -name "LVGLImage.py" | head -n1)
[ -z "$LV_CONV9" ] && LV_CONV9=$(find . -maxdepth 1 -name "LVGLImage.py" | head -n1)
[ -z "$LV_CONV9" ] && LV_CONV9=$(find ~ -maxdepth 2 -name "LVGLImage.py" | head -n1)
if ! [ -f "$LV_CONV9" ]
then
	>&2 echo "Could not find LVGLImage.py"
	>&2 echo "See https://github.com/lvgl/lvgl/blob/c9ba438eeb7a51a51a9a472ae186d40a7a13711c/scripts/LVGLImage.py"
	exit 1
fi

#LV_CONV9="$(dirname $0)/LVGLImage.py"

### CREATE OUTPUT DIRS ###

mkdir -p $DST_DIR
mkdir -p $DST_DIR/v8
mkdir -p $DST_DIR/v9

### CONVERT IMAGES ###

images="$(find $SRC_DIR -maxdepth 1 -regex '.+/[a-z_]+.svg')"
tmp=""

if [ 1 ]; then

for file in $images
do
    continue
	name=$(echo $(basename $file) | grep -o '^[^\.]*')
	echo $name

	rsvg-convert "$file" -w $SIZE -h $SIZE -f png -o "$DST_DIR/${name}.png.tmp"
	tmp="$tmp $DST_DIR/${name}.png.tmp"

	$LV_CONV8 "$DST_DIR/${name}.png.tmp" -f -c CF_TRUE_COLOR_ALPHA -i "mm_icon_$name" -o "$DST_DIR/v8/mm_icon_$name.c" &
	python3 $LV_CONV9 --cf A1 --ofmt C -o "$DST_DIR/v9" "$DST_DIR/${name}.png.tmp" --name "mm_icon_$name" &
	sleep 0.8
done

wait

echo "Removing$tmp"
rm $tmp

fi

### PRINT C CODE SNIPPETS ###

echo "==================== CMake sources variable ======================"

echo "set(SRC_IMAGES"
for file in $images
do
	name=$(echo $(basename $file) | grep -o '^[^\.]*')
	printf "        \"icons/v8/mm_icon_%s.c\"\n" "$name"
done
echo ")"

echo "==================== CMake sources variable ======================"

echo "/*            NAME                 STRING NAME          WIDTH RESIZEABLE */"
for file in $images
do
	name=$(echo $(basename $file) | grep -o '^[^\.]*')
    enum_name=$(echo $name | tr "[:lower:]" "[:upper:]")
	printf "FOR_EACH_ICON(%-19s, %-19s, %-2d, %-2d)\n" "$enum_name" "$name" $SIZE 1
done

echo "=================================================================="

#
#

#!/bin/sh
# Standalone build of the (partial) libui-ng Haiku backend plus the hello test.
# Run ON a Haiku machine, from the libui-ng repo root:   sh haiku/build-haiku.sh
# Produces ./libui-haiku-hello.
#
# Why not meson? The backend currently implements the event loop + window/button/label/box only.
# A full library() link pulls undefined uiDraw*/uiArea*/... from common/ (see haiku/meson.build).
# This script sidesteps that by compiling only the common files the 4-control backend links against
# (control.c, debug.c, shouldquit.c) — enough to build and run a real native window today.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OBJ="$ROOT/haiku/_obj"
rm -rf "$OBJ"; mkdir -p "$OBJ"

# BSpinner/BDecimalSpinner and BColumnListView are in private kits (headers + static libs).
PRIV="/boot/system/develop/headers/private/interface"
INC="-I. -I$PRIV"
# -lbe: Interface/App kits; -lshared: BSpinner; -ltracker: BFilePanel; -lcolumnlistview: BColumnListView.
LIBS="-lbe -lshared -ltracker -lcolumnlistview"

for f in common/control.c common/debug.c common/shouldquit.c common/tablevalue.c \
         common/attrstr.c common/attribute.c common/attrlist.c common/opentype.c common/utf.c; do
	echo "CC  $f"
	g++ $INC -c "$f" -o "$OBJ/common_$(basename ${f%.c}).o"
done
for f in haiku/*.cpp; do
	echo "CXX $f"
	g++ $INC -c "$f" -o "$OBJ/haiku_$(basename ${f%.cpp}).o"
done

echo "LD  libui-haiku-hello"
g++ $INC -o libui-haiku-hello haiku/test/hello.c "$OBJ"/*.o $LIBS
echo "LD  libui-haiku-widgets"
g++ $INC -o libui-haiku-widgets haiku/test/widgets.c "$OBJ"/*.o $LIBS
echo "LD  libui-haiku-menus"
g++ $INC -o libui-haiku-menus haiku/test/menus.c "$OBJ"/*.o $LIBS
echo "LD  libui-haiku-tabs"
g++ $INC -o libui-haiku-tabs haiku/test/tabs.c "$OBJ"/*.o $LIBS
echo "LD  libui-haiku-forms"
g++ $INC -o libui-haiku-forms haiku/test/forms.c "$OBJ"/*.o $LIBS
echo "LD  libui-haiku-draw"
g++ $INC -o libui-haiku-draw haiku/test/draw.c "$OBJ"/*.o $LIBS
echo "LD  libui-haiku-table"
g++ $INC -o libui-haiku-table haiku/test/table.c "$OBJ"/*.o $LIBS
echo "LD  libui-haiku-choosers"
g++ $INC -o libui-haiku-choosers haiku/test/choosers.c "$OBJ"/*.o $LIBS
echo "LD  libui-haiku-drawtext"
g++ $INC -o libui-haiku-drawtext haiku/test/drawtext.c "$OBJ"/*.o $LIBS
echo "BUILD OK -> $ROOT/libui-haiku-* (hello widgets menus tabs forms draw table choosers drawtext)"

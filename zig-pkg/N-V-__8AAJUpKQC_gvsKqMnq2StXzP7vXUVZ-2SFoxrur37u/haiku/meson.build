# Haiku backend (Interface Kit / BeAPI).
#
# STATUS: partial — implements the event loop + uiWindow, uiButton, uiLabel, uiBox. The remaining
# controls and the draw/area/text/menu/dialog APIs are not implemented yet, so the full library()
# link in the root meson.build will still report undefined uiDraw*/uiArea*/uiTable*/... symbols
# pulled in by common/. Until the backend is completed, the validated build is the standalone
# `haiku/build-haiku.sh` (it compiles only the common subset that links against this backend).
# This file is the integration point so finishing the backend is a matter of adding sources here.

libui_sources += [
	'haiku/alloc.cpp',
	'haiku/area.cpp',
	'haiku/box.cpp',
	'haiku/button.cpp',
	'haiku/checkbox.cpp',
	'haiku/colorbutton.cpp',
	'haiku/combobox.cpp',
	'haiku/control.cpp',
	'haiku/datetimepicker.cpp',
	'haiku/debug.cpp',
	'haiku/draw.cpp',
	'haiku/drawmatrix.cpp',
	'haiku/drawpath.cpp',
	'haiku/drawtext.cpp',
	'haiku/editablecombo.cpp',
	'haiku/entry.cpp',
	'haiku/fontbutton.cpp',
	'haiku/form.cpp',
	'haiku/grid.cpp',
	'haiku/group.cpp',
	'haiku/image.cpp',
	'haiku/label.cpp',
	'haiku/main.cpp',
	'haiku/menu.cpp',
	'haiku/multilineentry.cpp',
	'haiku/progressbar.cpp',
	'haiku/radiobuttons.cpp',
	'haiku/separator.cpp',
	'haiku/slider.cpp',
	'haiku/spinbox.cpp',
	'haiku/stddialogs.cpp',
	'haiku/tab.cpp',
	'haiku/table.cpp',
	'haiku/tablemodel.cpp',
	'haiku/text.cpp',
	'haiku/window.cpp',
]

libui_deps += [
	# The Be API (BApplication, BWindow, BView, BButton, ...).
	meson.get_compiler('cpp').find_library('be', required: true),
	# BSpinner/BDecimalSpinner live in the "shared" private kit (libshared.a).
	meson.get_compiler('cpp').find_library('shared', required: true),
	# BFilePanel (open/save dialogs) lives in libtracker.
	meson.get_compiler('cpp').find_library('tracker', required: true),
	# BColumnListView (uiTable) lives in libcolumnlistview.
	meson.get_compiler('cpp').find_library('columnlistview', required: true),
]
# Spinner headers are under the private interface kit.
add_project_arguments('-I/boot/system/develop/headers/private/interface', language: 'cpp')
libui_rpath = '$ORIGIN'

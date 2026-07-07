#pragma once
// RisalWidget.h — umbrella header for the widget model. The implementation is split by
// purpose under widgets/ so each family reads on its own; including this one header keeps
// the public API unchanged. Per-type CSS/JS PROGMEM strings live next to their widget class
// and are emitted once per served page (Zero-Waste: the linker strips unused types).
#include "widgets/base.h"
#include "widgets/icons.h"
#include "widgets/display.h"
#include "widgets/controls.h"
#include "widgets/inputs.h"
#include "widgets/visual.h"
#include "widgets/layout.h"

#include "c4d.h"
#include "main.h"
#include "c4d_symbols.h"
#include "customgui_rotaryknob.h"


/// Maps a value from an input range to an output range
/// @param[in] value The input value
/// @param[in] minInput Defines the lower limit of the input range
/// @param[in] maxInput Defines the upper limit of the input range
/// @param[in] minOutput Defines the lower limit of the output range
/// @param[in] maxOutput Defines the upper limit of the output range
/// @return The mapped value
static inline Float MapRange(Float value, Float minInput, Float maxInput, Float minOutput, Float maxOutput)
{
	if ((maxInput - minInput) == 0)
		return minOutput;
	
	return  minOutput + (maxOutput - minOutput) * ((value - minInput) / (maxInput - minInput));
}

/// Raster (or "round") a value to a grid.
/// This is an extended version of Round(), rounding not just to whole numbers but to a grid with any spacing.
/// The input value will be rounded to the nearest grid point.
/// @param[in] value The input value
/// @param[in] grid The grid spacing
/// @return The rounded value
static Float RoundGrid(Float value, Float grid)
{
	if (grid == 0.0)
		return 0.0;
	
	value = value / grid + 0.5;
	value = Floor(value);
	value = value * grid;
	
	return value;
}


KnobAreaDrawValues::KnobAreaDrawValues(GeClipMap &clipMap)
{
	// Area & background
	areaWidth = ROTARYKNOBAREA_WIDTH * ROTARYKNOBAREA_OVERSAMPLING;
	areaHalfWidth = areaWidth / 2;
	areaRadius = areaHalfWidth - ROTARYKNOBAREA_MARGIN;
	areaColor = GetGuiWorldColor(COLOR_BG);
	
	// Scale
	scaleRadius1 = areaRadius * 1.1;
	scaleRadius2 = areaRadius * 1.05;
	scaleLimitRadians = Rad(ROTARYKNOBAREA_SCALELIMIT);
	scaleLimitRadiansNeg = Rad(-ROTARYKNOBAREA_SCALELIMIT);
	scaleColor = GetGuiWorldColor(COLOR_BG_DARK1);
	
	// Knob
	knobOuterCorner1 = ROTARYKNOBAREA_MARGIN;
	knobOuterCorner2 = areaWidth - ROTARYKNOBAREA_MARGIN;
	knobOuterColor = GetGuiWorldColor(COLOR_BG_DARK1);
	
	knobInnerCorner1 = (Int32)(ROTARYKNOBAREA_MARGIN * 1.15);
	knobInnerCorner2 = areaWidth - knobInnerCorner1;
	knobInnerColor = GetGuiWorldColor(COLOR_BG_DARK2);
	
	knobCenterCorner1 = (Int32)(-ROTARYKNOBAREA_MARGIN + areaHalfWidth);
	knobCenterCorner2 = ROTARYKNOBAREA_MARGIN + areaHalfWidth;
	knobCenterColor = GetGuiWorldColor(COLOR_BG_HIGHLIGHT);
	
	// Marker
	markerLength = areaRadius * 0.8;
	markerThickness = ROTARYKNOBAREA_MARGIN * 0.6;
	markerColor = GetGuiWorldColor(COLOR_BG_HIGHLIGHT);
	
	// Value label
	labelPosY = (Int32)(areaHalfWidth * 1.5) - clipMap.GetTextHeight() / 2;
	labelColor = GetGuiWorldColor(COLOR_MENU_BG_ICON);
	clipMap.GetDefaultFont(GE_FONT_DEFAULT_SYSTEM, &labelFontDesc);
}


RotaryKnobArea::RotaryKnobArea() : _tristate(false), _value(0.0)
{
	if (_canvas)
	{
		// Prepare cache with values needed for drawing
		_drawValues = KnobAreaDrawValues(*_canvas);
	}
}

RotaryKnobArea::~RotaryKnobArea()
{}

Bool RotaryKnobArea::Init()
{
	return true;
}

Bool RotaryKnobArea::InitValues()
{
	return true;
}

Bool RotaryKnobArea::GetMinSize(Int32 &w, Int32 &h)
{
	w = h = ROTARYKNOBAREA_WIDTH;
	
	return true;
}

void RotaryKnobArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer &msg)
{
	if (!_canvas)
		return;
	
	// Cancel if anything goes wrong
	if (_canvas->Init(_drawValues.areaWidth, _drawValues.areaWidth, 24) != IMAGERESULT_OK)
		return;
	
	// Select whole user area as clipping area
	this->OffScreenOn();
	
	// Start drawing in ClipMap
	_canvas->BeginDraw();
	{
		// Fill the background
		DrawBackground(_drawValues);
		
		// Draw the scale
		DrawScale(_drawValues);
		
		// Draw the knob
		DrawKnob(_drawValues);
		
		// Draw the marker
		DrawMarker(_drawValues);
		
		// Draw the value
		DrawValue(_drawValues);
	}
	// End drawing in the ClipMap
	_canvas->EndDraw();
	
	// Draw ClipMap to user area
	this->DrawBitmap(_canvas->GetBitmap(), 0, 0, ROTARYKNOBAREA_WIDTH, ROTARYKNOBAREA_WIDTH, 0, 0, _drawValues.areaWidth, _drawValues.areaWidth, _tristate ? BMP_EMBOSSED : BMP_NORMAL);
}

Bool RotaryKnobArea::InputEvent(const BaseContainer &msg)
{
	const Int32 device  = msg.GetInt32(BFM_INPUT_DEVICE);
	const Int32 channel = msg.GetInt32(BFM_INPUT_CHANNEL);
	
	if (device == BFM_INPUT_MOUSE && channel == BFM_INPUT_MOUSELEFT)
	{
		// Catch double click first
		if (msg.GetBool(BFM_INPUT_DOUBLECLICK))
		{
			GePrint("Doubleclick!");
			
			// Notify parent GUI
			// Build message container with ID and value
			BaseContainer m(BFM_ACTION);
			m.SetInt32(BFM_ACTION_ID, GetId());
			m.SetData(BFM_ACTION_VALUE, GeData(_value));
			m.SetInt32(MSG_KNOBAREAMESSAGE, MSG_KNOBAREAMESSAGE_SHOWPOPUP);
			SendParentMessage(m);

			return true;
		}
		
		// Container we need later for the query calls
		BaseContainer channels;
		BaseContainer state;
		
		// Values required for calculating the deltas
		Int32 startX = msg.GetInt32(BFM_INPUT_X);  // Start X coordinate
		Int32 startY = msg.GetInt32(BFM_INPUT_Y);  // Start Y coordinate
		Float deltaX = 0.0;  // Current X delta (only needed to determine if mouse has moved during the drag)
		Float deltaY = 0.0;  // Current Y delta (no needed at all, but MouseDragStart() wants a Y delta, too)
		Float oldValue = _value;  // The value at the time when the mouse drag started
		
		Global2Local(&startX, &startY);  // Transform start coordinates to user area's local space
		
		// Start mouse drag
		MouseDragStart(BFM_INPUT_MOUSELEFT, startX, startY, MOUSEDRAGFLAGS_DONTHIDEMOUSE);
		
		// Check if mouse drag is still continueing
		while (MouseDrag(&deltaX, &deltaY, &channels) == MOUSEDRAGRESULT_CONTINUE)
		{
			// Get state of left mouse button
			if (!GetInputState(BFM_INPUT_MOUSE, BFM_INPUT_MOUSELEFT, state))
				break;
			
			// Cancel if mouse button not pressed
			if (state.GetInt32(BFM_INPUT_VALUE) == 0)
				break;
			
			// Mouse has been moved
			if (deltaX != 0.0 || deltaY != 0.0)
			{
				// Circular or linear knob behavior?
				if (_properties._circularMouse)
				{
					// We'll set the value according to the mouse cursor position in the circle
					
					// Steps:
					// 1. Transform the XY mouse coordinates to an angle
					// 2. Map the resulting angle back to the CustomGUI's value range
					// 3. Set result as _value
					
					// 1. Getting an angle
					
					// Get local mouse coordinates
					Int32 mouseX = state.GetInt32(BFM_INPUT_X);
					Int32 mouseY = state.GetInt32(BFM_INPUT_Y);
					Global2Local(&mouseX, &mouseY);
					
					// Construct vectors for angle measuretaking
					Vector coord((Float)mouseX, (Float)mouseY, 0.0);
					Vector unit(0.0, -1.0, 0.0);
					
					// Offset mouse coordinates, because the center of the knob is not at (0;0)
					Float halfWidth = (Float)ROTARYKNOBAREA_WIDTH / 2.0;
					coord.x -= halfWidth;
					coord.y -= halfWidth;
					
					// Normalize (required for the Atan2() angle measuring below)
					coord.Normalize();
					
					// Get angle in radians.
					Float angle = GetAngle(unit, coord);
					
					// Since GetAngle() always returns positive values, we have to get our sign back
					if (coord.x < 0.0)
						angle *= -1.0;

					// 2, Map the angle to the output range of the GUI element
					Float newValue = MapRange(angle, Rad(-ROTARYKNOBAREA_SCALELIMIT), Rad(ROTARYKNOBAREA_SCALELIMIT), _properties._descMin, _properties._descMax);
					
					//GePrint("MousePos=[" + String::FloatToString(coord.x) + ";" + String::FloatToString(coord.y) + "]; ScaleLimits=[" + String::FloatToString(-ROTARYKNOBAREA_SCALELIMIT) + "°;" + String::FloatToString(ROTARYKNOBAREA_SCALELIMIT) + "]; Angle=" + String::FloatToString(Deg(angle)) + "°; newValue=" + String::FloatToString(newValue));

					// 3. Set as new value
					_value = newValue;
				}
				else
				{
					// We calculate our own delta
					// The delta from the MouseDrag() call is only differential to the previous call,
					// but we need a delta that's differential to the ORIGINAL (not the previous) mouse position and the current one.
					Int32 currentY = state.GetInt32(BFM_INPUT_Y);
					Global2Local(nullptr, &currentY);
					Float totalDelta = startY - currentY;
					
					// Default multiplier (for knob rotating speed)
					Float multiplier = ROTARYKNOBAREA_MULTIPLIER_NORMAL;
					
					// Query SHIFT key. If it's pressed, we'll apply the PRECISE multiplier
					Bool bShift = channels.GetInt32(BFM_INPUT_QUALIFIER) & QSHIFT;
					if (bShift)
						multiplier = ROTARYKNOBAREA_MULTIPLIER_PRECISE; // Precise multiplier. Makes knob rotate slower.
					
					// Calculate the change of the value for this drag.
					// For this, we use the totalDelta and not the current delta (see above for more info)
					Float valueChange = totalDelta * (_properties._descMax - _properties._descMin) * multiplier;
					
					// Update value
					_value = oldValue + valueChange;
					
					// Calculate value grid snapping if CTRL is pressed
					Bool bCtrl = channels.GetInt32(BFM_INPUT_QUALIFIER) & QCTRL;
					if (bCtrl)
						_value = RoundGrid(_value, ROTARYKNOBAREA_VALUEGRIDSIZE);
				}
				
				// Clamp value, just to be on the safe side
				_value = ClampValue(_value, _properties._descMin, _properties._descMax);
				
				// Notify parent GUI
				// Build message container with ID and value
				BaseContainer m(BFM_ACTION);
				m.SetInt32(BFM_ACTION_ID, GetId());
				m.SetData(BFM_ACTION_VALUE, GeData(_value));
				m.SetBool(BFM_ACTION_INDRAG, true);  // Important: We're still dragging
				SendParentMessage(m);
			}
		}
		// Mouse drag is over now
		MouseDragEnd();
		return true;
	}
	
	return false;
}

void RotaryKnobArea::SetProperties(const DescElementProperties &properties)
{
	_properties = properties;
}

void RotaryKnobArea::SetValue(Float newValue, Bool newTristate)
{
	_value = newValue;
	_tristate = newTristate;
}

Float RotaryKnobArea::GetValue() const
{
	return _value;
}

void RotaryKnobArea::ColorToRGB(const Vector &color, Int32 &r, Int32 &g, Int32 &b) const
{
	r = (Int32)(color.x * 255.0);
	g = (Int32)(color.y * 255.0);
	b = (Int32)(color.z * 255.0);
}

void RotaryKnobArea::SetCanvasColor(const Vector &col)
{
	Int32 r = 0, g = 0, b = 0;
	
	// Get standard GUI color, convert to RGB
	ColorToRGB(col, r, g, b);
	
	// Set color
	_canvas->SetColor(r, g, b);
}

void RotaryKnobArea::DrawBackground(const KnobAreaDrawValues &drawValues)
{
	SetCanvasColor(drawValues.areaColor);
	_canvas->FillRect(0, 0, _canvas->GetBw(), _canvas->GetBw());
}

void RotaryKnobArea::DrawKnob(const KnobAreaDrawValues &drawValues)
{
	// Draw outer circle (acts as a bold dark outline)
	SetCanvasColor(drawValues.knobOuterColor);
	_canvas->FillEllipse(drawValues.knobOuterCorner1, drawValues.knobOuterCorner1, drawValues.knobOuterCorner2, drawValues.knobOuterCorner2);
	
	// Draw inner circle
	SetCanvasColor(drawValues.knobInnerColor);
	_canvas->FillEllipse(drawValues.knobInnerCorner1, drawValues.knobInnerCorner1, drawValues.knobInnerCorner2, drawValues.knobInnerCorner2);
	
	// Draw center
	SetCanvasColor(drawValues.knobCenterColor);
	_canvas->FillEllipse(drawValues.knobCenterCorner1, drawValues.knobCenterCorner1, drawValues.knobCenterCorner2, drawValues.knobCenterCorner2);
}

//TO DO: Still buggy
void RotaryKnobArea::DrawScale(const KnobAreaDrawValues &drawValues)
{
	// Draw n lines
	for (Int32 i = 0; i <= 10; ++i)
	{
		// Map value to scale
		Float value = MapRange((Float)i * 0.1, 0.0, 1.0, drawValues.scaleLimitRadiansNeg, drawValues.scaleLimitRadians);
		
		// Map value to circle
		Float x = Sin(value);
		Float y = Cos(value);
		
		// Select radius (every 2nd scale line is shorter)
		Float radius = (i % 2 == 1) ? drawValues.scaleRadius2 : drawValues.scaleRadius1;

		// Calculate draw coordinates
		Int32 ox = (Int32)(x * radius);
		Int32 oy = (Int32)(y * -radius);

		SetCanvasColor(drawValues.scaleColor);
		
		_canvas->Line(drawValues.areaHalfWidth, drawValues.areaHalfWidth, ox + drawValues.areaHalfWidth, oy + drawValues.areaHalfWidth);
	}
}

void RotaryKnobArea::DrawMarker(const KnobAreaDrawValues &drawValues)
{
	// Map value to scale
	Float value = MapRange(_value, _properties._descMin, _properties._descMax, drawValues.scaleLimitRadiansNeg, drawValues.scaleLimitRadians);
	
	// Map value to circle
	Float x = Sin(value);
	Float y = Cos(value);

	// Set color
	SetCanvasColor(drawValues.markerColor);
	
	// Set up point array
	GE_POINT2D points[3];
	points[0].x = (Int32)(y * drawValues.markerThickness + drawValues.areaHalfWidth);
	points[0].y = (Int32)(x * drawValues.markerThickness + drawValues.areaHalfWidth);
	points[1].x = (Int32)(-y * drawValues.markerThickness + drawValues.areaHalfWidth);
	points[1].y = (Int32)(-x * drawValues.markerThickness + drawValues.areaHalfWidth);
	points[2].x = (Int32)(x * drawValues.markerLength + drawValues.areaHalfWidth);
	points[2].y = (Int32)(y * -drawValues.markerLength + drawValues.areaHalfWidth);
	
	// Draw
	_canvas->FillPolygon(3, points);
}

// Draw value text
void RotaryKnobArea::DrawValue(KnobAreaDrawValues &drawValues)
{
	// Get value string
	String label = String::FloatToString(_value);
	
	// Set font size
	_canvas->SetFontSize(&drawValues.labelFontDesc, GE_FONT_SIZE_INTERNAL, ROTARYKNOBAREA_FONTSIZE);
	_canvas->SetFont(&drawValues.labelFontDesc);

	// Draw value string
	SetCanvasColor(drawValues.labelColor);
	_canvas->TextAt(drawValues.areaHalfWidth - _canvas->GetTextWidth(label) / 2, drawValues.labelPosY, label);
}



// Defining default values
RotaryKnobCustomGui::RotaryKnobCustomGui(const BaseContainer &settings, CUSTOMGUIPLUGIN *plugin) : iCustomGui(settings, plugin), _tristate(false), _value(0.0)
{
	_descProperties = DescElementProperties(settings);
}

// Rebuild layout
Bool RotaryKnobCustomGui::CreateLayout()
{
	GroupBegin(1000, BFH_SCALEFIT|BFV_FIT, 1, 2, String(), 0, ROTARYKNOBAREA_WIDTH, ROTARYKNOBAREA_WIDTH);
	{
		GroupSpace(0, 0);
		
		// Add element title
		if (!_descProperties._hideName)
			this->AddStaticText(0, BFH_CENTER, 0, 0, _descProperties._descName, 0);

		// Create the knob user area
		C4DGadget* userArea = this->AddUserArea(IDC_KNOBAREA, BFH_CENTER, ROTARYKNOBAREA_WIDTH, ROTARYKNOBAREA_WIDTH);
		this->AttachUserArea(_knob, userArea);
		
		// Set data in user area
		_knob.SetProperties(_descProperties);
		_knob.SetValue(_value, _tristate);
	}
	GroupEnd();

	return SUPER::CreateLayout();
}

// Set values to GUI elements, handle tristates
Bool RotaryKnobCustomGui::InitValues()
{
	// Dummy TriState
	TriState<Float> triState;
	triState.Add(_value);
	
	// If GUI element is in a tristate state, just add some dummy value to the number field look tristate'y
	if (_tristate)
		triState.Add(123.456);
	
	// Set tristate to number field and knob area
	_knob.SetValue(_value, _tristate);

	return SUPER::InitValues();
}

// An element was used by the user
Bool RotaryKnobCustomGui::Command(Int32 id, const BaseContainer &msg)
{
	switch (id)
	{
		case IDC_KNOBAREA:
		{
			// Catch our custom KnobArea messages first
			if (msg.GetInt32(MSG_KNOBAREAMESSAGE) == MSG_KNOBAREAMESSAGE_SHOWPOPUP)
			{
				GePrint("Show value entry popup!");
				return true;
			}
			
			// Get new value from knob user area
			_value = _knob.GetValue();
			
			// Update GUI
			this->InitValues();
			
			// Send message to parent object to update the parameter value
			SendParentGuiMessage();
			
			return true;
		}
	}

	return SUPER::Command(id, msg);
}

Int32 RotaryKnobCustomGui::Message(const BaseContainer& msg, BaseContainer& result)
{
	// Cinema wants to get cursor information
	if (msg.GetId() == BFM_GETCURSORINFO)
	{
		// Construct result message container
		result.SetId(BFM_GETCURSORINFO);
		result.SetInt32(RESULT_CURSOR, MOUSE_POINT_HAND);
		result.SetString(RESULT_BUBBLEHELP, String::FloatToString(_value));
	}
	
	return SUPER::Message(msg, result);
}

// The data is changed from the outside.
Bool RotaryKnobCustomGui::SetData(const TriState<GeData> &tristate)
{
	// Store values internally
	_value    = tristate.GetValue().GetFloat();
	_tristate = tristate.GetTri();

	// Set values to GUI elements & trigger redraw
	this->InitValues();
	_knob.Redraw();

	return true;
}

// The data is requested from the outside.
TriState<GeData> RotaryKnobCustomGui::GetData()
{
	// Construct a TriState from the value
	TriState<GeData> tri;
	tri.Add(_value);

	return tri;
}

Int32 RotaryKnobCustomGui::CustomGuiWidth()
{
	return ROTARYKNOBAREA_WIDTH;
}

Int32 RotaryKnobCustomGui::CustomGuiHeight()
{
	return ROTARYKNOBAREA_WIDTH;
}

void RotaryKnobCustomGui::SendParentGuiMessage()
{
	// Build message container with ID and value
	BaseContainer m(BFM_ACTION);
	m.SetInt32(BFM_ACTION_ID, GetId());
	m.SetData(BFM_ACTION_VALUE, GeData(_value));
	
	// Send message
	SendParentMessage(m);
}



Int32 RotaryKnobCustomGuiData::GetId()
{
	return ID_CUSTOMGUI_ROTARYKNOB;
};

CDialog* RotaryKnobCustomGuiData::Alloc(const BaseContainer &settings)
{
	RotaryKnobCustomGui *dlg = NewObj(RotaryKnobCustomGui, settings, GetPlugin());
  if (!dlg) 
		return nullptr;

  return dlg->Get();
};

void RotaryKnobCustomGuiData::Free(CDialog *dlg, void *userdata)
{
	if (!dlg || !userdata) 
		return;

  RotaryKnobCustomGui *sub = static_cast<RotaryKnobCustomGui*>(userdata);
  DeleteObj(sub);
};

// Return the resource symbol. This symbol can be used in resource files in combination with "CUSTOMGUI"
const Char *RotaryKnobCustomGuiData::GetResourceSym()
{
	return SYM_ROTARYKNOB;
};

// Return the pointer to a data structure holding the CustomGUI's custom properties
CustomProperty *RotaryKnobCustomGuiData::GetProperties()
{
	return g_RotaryKnobProps;
};

// Return the applicable datatypes defined in the stringtable array
Int32 RotaryKnobCustomGuiData::GetResourceDataType(Int32 *&table)
{
	table = stringtable; 
	return sizeof(stringtable) / sizeof(Int32);
};


// Register the CustomGUI
Bool RegisterRotaryKnobCustomGui()
{
	// Declare, allocate and fill the CustomGUI library
	static BaseCustomGuiLib rotaryKnobGUIlib;
	ClearMem(&rotaryKnobGUIlib, sizeof(rotaryKnobGUIlib));
	FillBaseCustomGui(rotaryKnobGUIlib);

	// Install the CustomGUI library
	if (!InstallLibrary(ID_CUSTOMGUI_ROTARYKNOB, &rotaryKnobGUIlib, 1000, sizeof(rotaryKnobGUIlib)))
		return false;
	
	// Register the CustomGUI
	if (!RegisterCustomGuiPlugin(GeLoadString(IDS_CUSTOMGUI_ROTARYKNOB), 0, NewObjClear(RotaryKnobCustomGuiData)))
		return false;

	return true;
}

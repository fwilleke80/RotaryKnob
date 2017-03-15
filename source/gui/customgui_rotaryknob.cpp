#include "c4d.h"
#include "main.h"
#include "lib_clipmap.h"
#include "c4d_symbols.h"
#include "customgui_rotaryknob.h"


// Maps a value from an input range to an output range
static inline Float MapRange(Float value, Float minInput, Float maxInput, Float minOutput, Float maxOutput)
{
	if ((maxInput - minInput) == 0)
		return minOutput;
	
	return  minOutput + (maxOutput - minOutput) * ((value - minInput) / (maxInput - minInput));
}

static Float Raster(Float value, Float grid)
{
	if (grid == 0.0)
		return 0.0;
	
	value = value / grid + 0.5;
	value = Floor(value);
	value = value * grid;
	
	return value;
}


RotaryKnobArea::RotaryKnobArea() : _tristate(false), _value(0.0)
{}

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
	// Calculate area width
	Int32 width = ROTARYKNOBAREA_WIDTH * ROTARYKNOBAREA_OVERSAMPLING;
	
	// Cancel if anything goes wrong
	if (!_canvas || _canvas->Init(width, width, 24) != IMAGERESULT_OK)
		return;
	
	// Select whole user area as clipping area
	this->OffScreenOn();
	
	// Start drawing in ClipMap
	_canvas->BeginDraw();
	{
		// Fill the background
		DrawBackground();
		
		// Draw the knob
		DrawKnob();
		
		// Draw the marker
		DrawMarker();
		
		// Draw the value, if desired
		if (_properties._valueInKnob)
			DrawValue();
	}
	// End drawing in the ClipMap
	_canvas->EndDraw();
	
	// Draw ClipMap to user area
	this->DrawBitmap(_canvas->GetBitmap(), 0, 0, ROTARYKNOBAREA_WIDTH, ROTARYKNOBAREA_WIDTH, 0, 0, width, width, _tristate ? BMP_EMBOSSED : BMP_NORMAL);
}

// TODO: Rather use GetInputState() to achieve a loop while mouse is held down
Bool RotaryKnobArea::InputEvent(const BaseContainer &msg)
{
	const Int32 device  = msg.GetInt32(BFM_INPUT_DEVICE);
	const Int32 channel = msg.GetInt32(BFM_INPUT_CHANNEL);
	
	if (device == BFM_INPUT_MOUSE && channel == BFM_INPUT_MOUSELEFT)
	{
		BaseContainer channels;
		BaseContainer state;
		Int32 startX = msg.GetInt32(BFM_INPUT_X);
		Int32 startY = msg.GetInt32(BFM_INPUT_Y);
		Float deltaX = 0.0;
		Float deltaY = 0.0;
		Float oldValue = _value;
		
		Global2Local(&startX, &startY);
		
		MouseDragStart(BFM_INPUT_MOUSELEFT, startX, startY, MOUSEDRAGFLAGS_DONTHIDEMOUSE);
		while (MouseDrag(&deltaX, &deltaY, &channels) == MOUSEDRAGRESULT_CONTINUE)
		{
			// Get state of left mouse button
			if (!GetInputState(BFM_INPUT_MOUSE, BFM_INPUT_MOUSELEFT, state))
				break;
			
			// Cancel if mouse button not pressed
			if (state.GetInt32(BFM_INPUT_VALUE) == 0)
				break;
			
			// Mouse has been moved
			if (deltaY != 0.0)
			{
				if (_properties._circularMouse)
				{
					// We'll set the value according to the mouse cursor position in the circle
					
				}
				else
				{
					// We calculate our own delta
					// The delta from the MouseDrag() call is only differential to the previous call,
					// but we need a delta that's differential to the ORIGINAL (not the previous) mouse position and the current one.
					Int32 currentY = state.GetInt32(BFM_INPUT_Y);
					Global2Local(nullptr, &currentY);
					Float totalDelta = startY - currentY;
					
					// Default multiplier
					Float multiplier = ROTARYKNOBAREA_MULTIPLIER_NORMAL;
					
					// Query SHIFT key. If it's pressed, we'll apply the PRECISE multiplier
					Bool bShift = channels.GetInt32(BFM_INPUT_QUALIFIER) & QSHIFT;
					if (bShift)
						multiplier = ROTARYKNOBAREA_MULTIPLIER_PRECISE;
					
					// Calculate the change of the value for this drag
					Float valueChange = totalDelta * (_properties._descMax - _properties._descMin) * multiplier;
					_value = oldValue + valueChange;
				}
				
				// Clamp value, just to be on the safe side
				_value = ClampValue(_value, _properties._descMin, _properties._descMax);
				
				// Calculate value grid snapping if CTRL is pressed
				Bool bCtrl = channels.GetInt32(BFM_INPUT_QUALIFIER) & QCTRL;
				if (bCtrl)
					_value = Raster(_value, ROTARYKNOBAREA_VALUEGRIDSIZE);
				
				// Notify parent GUI
				BaseContainer m(BFM_ACTION);
				m.SetInt32(BFM_ACTION_ID, GetId());
				m.SetFloat(BFM_ACTION_VALUE, _value);
				m.SetBool(BFM_ACTION_INDRAG, true);  // Important: We're still dragging
				SendParentMessage(m);
			}
		}
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

void RotaryKnobArea::GetValueCoords(Float &x, Float &y) const
{
	x = Cos(_value / PI);
	y = Sin(_value / PI);
}

void RotaryKnobArea::ColorToRGB(const Vector &color, Int32 &r, Int32 &g, Int32 &b) const
{
	r = (Int32)(color.x * 255.0);
	g = (Int32)(color.y * 255.0);
	b = (Int32)(color.z * 255.0);
}

void RotaryKnobArea::SetCanvasColor(Int32 cid)
{
	Int32 r = 0, g = 0, b = 0;
	
	ColorToRGB(GetGuiWorldColor(cid), r, g, b);
	_canvas->SetColor(r, g, b);
}

void RotaryKnobArea::DrawBackground()
{
	SetCanvasColor(COLOR_BG);
	_canvas->FillRect(0, 0, _canvas->GetBw(), _canvas->GetBw());
}

void RotaryKnobArea::DrawKnob()
{
	Int32 width = ROTARYKNOBAREA_WIDTH * ROTARYKNOBAREA_OVERSAMPLING;
	Int32 halfWidth = width / 2;
	
	SetCanvasColor(COLOR_BG_DARK1);
	_canvas->FillEllipse(ROTARYKNOBAREA_MARGIN, ROTARYKNOBAREA_MARGIN, width - ROTARYKNOBAREA_MARGIN, width - ROTARYKNOBAREA_MARGIN);
	
	SetCanvasColor(COLOR_BG_DARK2);
	_canvas->FillEllipse((Int32)(ROTARYKNOBAREA_MARGIN * 1.2), (Int32)(ROTARYKNOBAREA_MARGIN * 1.2), (Int32)(width - ROTARYKNOBAREA_MARGIN * 1.2), (Int32)(width - ROTARYKNOBAREA_MARGIN * 1.2));
	
	SetCanvasColor(COLOR_BG_HIGHLIGHT);
	_canvas->FillEllipse(-ROTARYKNOBAREA_MARGIN + halfWidth, -ROTARYKNOBAREA_MARGIN + halfWidth, ROTARYKNOBAREA_MARGIN + halfWidth, ROTARYKNOBAREA_MARGIN + halfWidth);
}

void RotaryKnobArea::DrawMarker()
{
	// Input scale
	Float input_min = _properties._descMin;
	Float input_max = _properties._descMax;
	
	// Output scale
	Float scale_min = Rad(-225.0);
	Float scale_max = Rad(45.0);
	
	// Map value to scale
	Float value = MapRange(_value, input_min, input_max, scale_min, scale_max);
	
	// Map value to circle
	Float x = Cos(value);
	Float y = Sin(value);
	
	// Calculate draw parameters
	Float width = ROTARYKNOBAREA_WIDTH * ROTARYKNOBAREA_OVERSAMPLING;
	Float halfWidth = width / 2;
	Float radius = halfWidth - ROTARYKNOBAREA_MARGIN;
	Float radius08 = radius * 0.8;
	
	// Calculate draw coordinates
	Float ox = x * radius08;
	Float oy = y * radius08;
	
	// Set color
	SetCanvasColor(COLOR_BG_HIGHLIGHT);
	
	// Draw
	_canvas->Line((Int32)halfWidth, (Int32)halfWidth, (Int32)(ox + halfWidth), (Int32)(oy + halfWidth));
}

// Draw value text
void RotaryKnobArea::DrawValue()
{
	// Calculate draw parameters
	Int32 width = ROTARYKNOBAREA_WIDTH * ROTARYKNOBAREA_OVERSAMPLING;
	Int32 halfWidth = width / 2;
	
	// Get value string
	String label = String::FloatToString(_value);
	
	// Set font size
	BaseContainer fontDesc;
	if (!_canvas->GetDefaultFont(GE_FONT_DEFAULT_SYSTEM, &fontDesc))
		return;
	_canvas->SetFontSize(&fontDesc, GE_FONT_SIZE_INTERNAL, ROTARYKNOBAREA_FONTSIZE);
	_canvas->SetFont(&fontDesc);

	// Draw value string
	SetCanvasColor(COLOR_MENU_BG_ICON);
	_canvas->TextAt(halfWidth - _canvas->GetTextWidth(label) / 2, (Int32)(halfWidth * 1.5) - _canvas->GetTextHeight() / 2, label);
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

		// Add number field for value display
		if (!_descProperties._hideValue && !_descProperties._valueInKnob)
			AddEditNumber(IDC_VALUE, BFH_CENTER, ROTARYKNOBAREA_WIDTH, 10);
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
	this->SetFloat(IDC_VALUE, triState, _descProperties._descMin, _descProperties._descMax, _descProperties._descStep);
	_knob.SetValue(_value, _tristate);

	return true;
}

// An element was used by the user
Bool RotaryKnobCustomGui::Command(Int32 id, const BaseContainer &msg)
{
	switch (id)
	{
		case IDC_KNOBAREA:
		{
			// Get new value from knob user area
			_value = _knob.GetValue();
			
			// Update GUI
			this->InitValues();
			
			// Send message to parent object to update the parameter value
			SendParentGuiMessage();
			
			return true;
		}

		case IDC_VALUE:
		{
			// Get new value from number field
			_value = msg.GetFloat(BFM_ACTION_VALUE);
			
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
	if (msg.GetId() == BFM_GETCURSORINFO)
	{
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
	// Send message to parent object to update the parameter value
	BaseContainer m(BFM_ACTION);
	m.SetInt32(BFM_ACTION_ID, GetId());
	m.SetData(BFM_ACTION_VALUE, GeData(_value));
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

// Destroys the given subdialog.
void RotaryKnobCustomGuiData::Free(CDialog *dlg, void *userdata)
{
	if (!dlg || !userdata) 
		return;

  RotaryKnobCustomGui *sub = static_cast<RotaryKnobCustomGui*>(userdata);
  DeleteObj(sub);
};

// Returns the resource symbol. This symbol can be used in resource files in combination with "CUSTOMGUI".
const Char *RotaryKnobCustomGuiData::GetResourceSym()
{
	return SYM_ROTARYKNOB;
};

// This method can return a pointer to a data structure holding various additional properties.
CustomProperty *RotaryKnobCustomGuiData::GetProperties()
{
	return g_RotaryKnobProps;
};

// Returns the applicable datatypes defined in the stringtable array.
Int32 RotaryKnobCustomGuiData::GetResourceDataType(Int32 *&table)
{
	table = stringtable; 
	return sizeof(stringtable) / sizeof(Int32);
};


/// Register the CustomGUI
Bool RegisterRotaryKnobCustomGui()
{
	static BaseCustomGuiLib rotaryKnobGUIlib;

	ClearMem(&rotaryKnobGUIlib, sizeof(rotaryKnobGUIlib));
	FillBaseCustomGui(rotaryKnobGUIlib);

	if (!InstallLibrary(ID_CUSTOMGUI_ROTARYKNOB, &rotaryKnobGUIlib, 1000, sizeof(rotaryKnobGUIlib)))
		return false;
	
	if (!RegisterCustomGuiPlugin(GeLoadString(IDS_CUSTOMGUI_ROTARYKNOB), 0, NewObjClear(RotaryKnobCustomGuiData)))
		return false;

	return true;
}

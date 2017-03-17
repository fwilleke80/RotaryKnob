#ifndef CUSTOMGUI_ROTARYKNOB_H__
#define CUSTOMGUI_ROTARYKNOB_H__

#include "c4d.h"


/// Plugin ID for Rotary Knob CustomGUI
static const Int32 ID_CUSTOMGUI_ROTARYKNOB = 1038994;

// IDs for GUI elements
static const Int32 IDC_KNOBAREA = 1001;  ///< The ID of the knob user area
static const Int32 IDC_VALUE    = 1002;  ///< The ID of the value display

/// ID values for Rotary Knob CustomProperties
enum
{
	ROTARY_HIDE_VALUE	= 10000,       ///< Hide the value display
	ROTARY_VALUE_IN_KNOB = 10001,    ///< Draw the value inside the knob instead of in a separate number field
	ROTARY_HIDE_NAME = 10002,        ///< Hide the parameter name above the knob
	ROTARY_CIRCULARMOUSE = 10003     ///< Use circular instead of linear mouse movement
};

/// CustomProperties for Rotary Knob CustomGUI
CustomProperty g_RotaryKnobProps[] =
{
	{ CUSTOMTYPE_FLAG, ROTARY_HIDE_VALUE, "HIDE_VALUE" },
	{ CUSTOMTYPE_FLAG, ROTARY_VALUE_IN_KNOB, "VALUE_IN_KNOB" },
	{ CUSTOMTYPE_FLAG, ROTARY_HIDE_NAME, "HIDE_NAME" },
	{ CUSTOMTYPE_FLAG, ROTARY_CIRCULARMOUSE, "CIRCULAR" },
	{ CUSTOMTYPE_END, 0, "" }
};

// Some more stuff
#define SYM_ROTARYKNOB "ROTARYKNOB"          ///< Resource symbol for use with "CUSTOMGUI" resource property
static Int32 stringtable[] = { DTYPE_REAL }; ///< This array defines the applicable datatypes


// Some internal constants
static const Int32 ROTARYKNOBAREA_WIDTH = 100;       ///< Standard width of CustomGUI
static const Int32 ROTARYKNOBAREA_MARGIN = 10;       ///< Size of margin between knob and border of user area
static const Int32 ROTARYKNOBAREA_OVERSAMPLING = 2;  ///< Oversampling for the Knob area. The bigger the value, the better the quality (and the slower the drawing)
static const Float ROTARYKNOBAREA_MULTIPLIER_NORMAL = 0.01;   ///< Normal knob move speed
static const Float ROTARYKNOBAREA_MULTIPLIER_PRECISE = 0.001; ///< Precise (slow) knob move speed
static const Int32 ROTARYKNOBAREA_FONTSIZE = 28;        ///< Font size for the value display with VALUE_IN_KNOB
static const Float ROTARYKNOBAREA_VALUEGRIDSIZE = 0.5;  ///< Grid size for value snapping during mouse drag
static const Float ROTARYKNOBAREA_SCALEBEGIN = -225.0;  ///< Where the usable range of the rotary knob starts
static const Float ROTARYKNOBAREA_SCALEEND = 45.0;      ///< Where the usable range of the rotary knob ends


/// This struct holds some of the DESC_ properties required for the rotary knob user area
struct DescElementProperties
{
	Bool  _hideValue;      ///< Hide value display
	Bool  _valueInKnob;    ///< Draw the value inside the knob instead of in a separate number field
	Bool  _hideName;       ///< Don't draw the name on top of the knob
	Bool  _circularMouse;  ///< Use circular instead of linear mouse movement
	Float _descMin;        ///< Min value
	Float _descMax;        ///< Max value
	Float _descStep;       ///< Step size
	String _descName;      ///< Element name
	
	/// Default constructor
	DescElementProperties() : _hideValue(false), _valueInKnob(false), _hideName(false), _circularMouse(false), _descMin(0.0), _descMax(0.0), _descStep(0.0)
	{}
	
	/// Construct from BaseContainer with DESC_ properties
	DescElementProperties(const BaseContainer &src)
	{
		_hideValue = src.GetBool(ROTARY_HIDE_VALUE);
		_valueInKnob = src.GetBool(ROTARY_VALUE_IN_KNOB);
		_circularMouse = src.GetBool(ROTARY_CIRCULARMOUSE);
		_hideName = src.GetBool(ROTARY_HIDE_NAME);
		_descMin = src.GetFloat(DESC_MIN, 0.0);
		_descMax = src.GetFloat(DESC_MAX, 0.0);
		_descStep = src.GetFloat(DESC_STEP, 0.0);
		_descName = src.GetString(DESC_NAME);
	}
};


/// The user area used to display the actual rotary knob.
/// It also handles all mouse input on the knob, and has a GeClipMap for nice drawing capabilities.
class RotaryKnobArea : public GeUserArea
{
	INSTANCEOF(RotaryKnobArea, GeUserArea);
	
public:
	RotaryKnobArea();
	virtual ~RotaryKnobArea();
	
	virtual Bool Init();
	virtual Bool InitValues();
	virtual Bool GetMinSize(Int32 &w, Int32 &h);
	
	virtual void DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer &msg);
	virtual Bool InputEvent(const BaseContainer &msg);
	
	/// Set properties
	/// @param[in] properties Ref to a DescElementProperties object
	void SetProperties(const DescElementProperties &properties);
	
	/// Sets a new value
	/// @param[in] newValue The new value
	/// @param[in] newTristate The new tristate
	void SetValue(Float newValue, Bool newTristate = false);
	
	/// Return the current value
	Float GetValue() const;
	
private:
	/// Get the corresponding X and Y coordinates for the current value
	/// @param[in] x Assigned the resulting X coordinate
	/// @param[in] y Assigned the resulting Y coordinate
	void GetValueCoords(Float &x, Float &y) const;
	
	/// Converts a color vector (0.0 ... 1.0) to separate RGB values (0 ... 255)
	void ColorToRGB(const Vector &color, Int32 &r, Int32 &g, Int32 &b) const;
	
	/// Sets a standard GUI color as draw color in the GeClipMap
	/// param[in] cid A color ID from the C4D API's COLOR_ enumeration
	void SetCanvasColor(Int32 cid);
	
	/// Draw the knob background
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawBackground();
	
	/// Draw the knob
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawKnob();
	
	/// Draw the knob's marker
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawMarker();
	
	/// Draw the value on the knob
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawValue();
	
private:
	Bool       _tristate;  ///< True, if the GUI element is in a tristate
	Float      _value;     ///< The value
	DescElementProperties  _properties;  ///< Custom properties as specified in the .res file
	AutoAlloc<GeClipMap>   _canvas;      ///< GeClipMap for drawing the knob
};


/// A custom GUI to display a REAL value as a rotary knob
/// This class implements the actual CustomGUI, including layout, value getting/setting, et cetera.
class RotaryKnobCustomGui : public iCustomGui
{
	INSTANCEOF(RotaryKnobCustomGui, iCustomGui);
	
public:
	RotaryKnobCustomGui(const BaseContainer &settings, CUSTOMGUIPLUGIN *plugin);
	virtual Bool CreateLayout();
	virtual Bool InitValues();
	virtual Bool Command(Int32 id, const BaseContainer &msg);
	virtual Int32 Message(const BaseContainer& msg, BaseContainer& result);
	virtual Bool SetData(const TriState<GeData> &tristate);
	virtual TriState<GeData> GetData();
	virtual Int32 CustomGuiWidth();
	virtual Int32 CustomGuiHeight();
	
	/// Simply send a BFM_ACTION message with our ID and value to the parent GUI element
	void SendParentGuiMessage();
	
private:
	Float   _value;        ///< The current value
	Bool    _tristate;     ///< The current tristate
	
	RotaryKnobArea         _knob;            ///< The knob user area
	DescElementProperties  _descProperties;  ///< Properties of the description element
};


/// This CustomGuiData class registers the rotary knob as a new custom GUI for the REAL datatype.
class RotaryKnobCustomGuiData : public CustomGuiData
{
	INSTANCEOF(RotaryKnobCustomGuiData, CustomGuiData);
	
public:
	virtual Int32 GetId();
	virtual CDialog *Alloc(const BaseContainer &settings);
	virtual void Free(CDialog *dlg, void *userdata);
	virtual const Char *GetResourceSym();
	virtual CustomProperty *GetProperties();
	virtual Int32 GetResourceDataType(Int32 *&table);
};


#endif  // CUSTOMGUI_ROTARYKNOB_H__

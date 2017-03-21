#ifndef CUSTOMGUI_ROTARYKNOB_H__
#define CUSTOMGUI_ROTARYKNOB_H__

#include "c4d.h"
#include "lib_clipmap.h"


/// Plugin ID for Rotary Knob CustomGUI
static const Int32 ID_CUSTOMGUI_ROTARYKNOB = 1038994;

// IDs for GUI elements
static const Int32 IDC_KNOBAREA = 1001;  ///< The ID of the knob user area

// IDs for GUI messages
static const Int32 MSG_KNOBAREAMESSAGE = 1039007;      ///< Unique ID for messages from the KnobArea to the CustomGUI
static const Int32 MSG_KNOBAREAMESSAGE_SHOWPOPUP = 1;  ///< Show value entry popup

/// ID values for Rotary Knob CustomProperties
enum
{
	ROTARY_HIDE_NAME = 10001,        ///< Hide the parameter name above the knob
	ROTARY_CIRCULARMOUSE = 10002     ///< Use circular instead of linear mouse movement
};

/// CustomProperties for Rotary Knob CustomGUI
CustomProperty g_RotaryKnobProps[] =
{
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
static const Float ROTARYKNOBAREA_SCALELIMIT = 135.0;  ///< Where the usable range of the rotary knob starts and ends


/// This struct holds some of the DESC_ properties required for the rotary knob user area
struct DescElementProperties
{
	Bool  _hideName;       ///< Don't draw the name on top of the knob
	Bool  _circularMouse;  ///< Use circular instead of linear mouse movement
	Float _descMin;        ///< Min value
	Float _descMax;        ///< Max value
	Float _descStep;       ///< Step size
	String _descName;      ///< Element name
	
	/// Default constructor
	DescElementProperties() : _hideName(false), _circularMouse(false), _descMin(0.0), _descMax(0.0), _descStep(0.0)
	{}
	
	/// Construct from BaseContainer with DESC_ properties
	DescElementProperties(const BaseContainer &src)
	{
		_circularMouse = src.GetBool(ROTARY_CIRCULARMOUSE);
		_hideName = src.GetBool(ROTARY_HIDE_NAME);
		_descMin = src.GetFloat(DESC_MIN, 0.0);
		_descMax = src.GetFloat(DESC_MAX, 0.0);
		_descStep = src.GetFloat(DESC_STEP, 0.0);
		_descName = src.GetString(DESC_NAME);
	}
};


/// This struct holds some values that will be used throughout the drawing
/// functions, so those values don't have to be calculated unnecessarily often.
struct KnobAreaDrawValues
{
	Int32 areaWidth;
	Int32 areaHalfWidth;
	Float areaRadius;
	Vector areaColor;
	
	Float scaleRadius1;
	Float scaleRadius2;
	Vector scaleColor;
	
	Int32 knobOuterCorner1;
	Int32 knobOuterCorner2;
	Vector knobOuterColor;
	
	Int32 knobInnerCorner1;
	Int32 knobInnerCorner2;
	Vector knobInnerColor;
	
	Int32 knobCenterCorner1;
	Int32 knobCenterCorner2;
	Vector knobCenterColor;
	
	Float markerRadius;
	Vector markerColor;
	
	Int32 labelPosY;
	Vector labelColor;
	BaseContainer labelFontDesc;
	
	
	KnobAreaDrawValues() : areaWidth(0), areaHalfWidth(0), areaRadius(0.0), scaleRadius1(0.0), scaleRadius2(0.0), knobOuterCorner1(0), knobOuterCorner2(0), knobInnerCorner1(0), knobInnerCorner2(0), knobCenterCorner1(0), knobCenterCorner2(0), markerRadius(0.0), labelPosY(0)
	{}
	
	KnobAreaDrawValues(GeClipMap &clipMap);
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
	/// Converts a color vector (0.0 ... 1.0) to separate RGB values (0 ... 255)
	void ColorToRGB(const Vector &color, Int32 &r, Int32 &g, Int32 &b) const;
	
	/// Sets a color as draw color in the GeClipMap
	/// param[in] col The color to set
	void SetCanvasColor(const Vector &col);
	
	/// Draw the knob background
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawBackground(const KnobAreaDrawValues &drawValues);
	
	/// Draw the knob
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawKnob(const KnobAreaDrawValues &drawValues);
	
	/// Draw the scale
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawScale(const KnobAreaDrawValues &drawValues);
	
	/// Draw the knob's marker
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawMarker(const KnobAreaDrawValues &drawValues);
	
	/// Draw the value on the knob
	/// @note: Must be called between BeginDraw() and EndDraw()
	void DrawValue(KnobAreaDrawValues &drawValues);
	
private:
	Bool       _tristate;  ///< True, if the GUI element is in a tristate
	Float      _value;     ///< The value
	DescElementProperties  _properties;  ///< Custom properties as specified in the .res file
	AutoAlloc<GeClipMap>   _canvas;      ///< GeClipMap for drawing the knob
	KnobAreaDrawValues     _drawValues;  ///< Cache for values used during drawing
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

#include "c4d.h"
#include "main.h"


#define PLUGIN_VERSION String("RotaryKnob 0.2")


Bool PluginStart()
{
	// Rotary knob custom gui
	if (!RegisterRotaryKnobCustomGui())
		return false;

	// Test object
	if (!RegisterTestObject())
		return false;

	GePrint(PLUGIN_VERSION);
	
	return true;
}

void PluginEnd()
{}

Bool PluginMessage(Int32 id, void* data)
{
	switch (id)
	{
		case C4DPL_INIT_SYS:
			if (!resource.Init())
				return false;		// don't start plugin without resource
	}

	return false;
}

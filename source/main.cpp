#include "c4d.h"
#include "main.h"


Bool PluginStart()
{
	// Rotary knob custom gui
	if (!RegisterRotaryKnobCustomGui())
		return false;

	// Test object
	if (!RegisterTestObject())
		return false;
	
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

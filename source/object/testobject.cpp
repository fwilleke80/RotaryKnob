#include "c4d.h"
#include "c4d_symbols.h"
#include "otest.h"
#include "main.h"

const Int32 ID_TESTOBJECT = 1038993;


/// This plugin implements an object that does absolutely nothing,
/// it acts simple as a test environment for the CustomGUI.
class TestObjectData : public ObjectData
{
	INSTANCEOF(TestObjectData, ObjectData)

public:
	virtual Bool Init(GeListNode* node);

	static NodeData* Alloc();
};


Bool TestObjectData::Init(GeListNode* node)
{
	BaseObject *op = static_cast<BaseObject*>(node);
	BaseContainer *data = op->GetDataInstance();
	if (!data)
		return false;

	data->SetFloat(TEST_PARAM_1, 0.50);
	data->SetFloat(TEST_PARAM_2, 2.25);
	data->SetFloat(TEST_PARAM_3, 3.75);

	return true;
}

NodeData *TestObjectData::Alloc()
{
	return NewObjClear(TestObjectData);
}


Bool RegisterTestObject()
{
	return RegisterObjectPlugin(ID_TESTOBJECT, GeLoadString(IDS_TESTOBJECT), 0, TestObjectData::Alloc, "Otest", AutoBitmap("otest.tif"), 0);
}

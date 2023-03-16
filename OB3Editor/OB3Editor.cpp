
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include <algorithm>
#include <cctype>

#include <unordered_map>
#include <unordered_set>

#include <Files.h>
#include <ObjectDescriptors.h>
#include <KnownObjs.h>

using namespace std;

bool usingMetres = false;

#define GAME_UNITS_IN_METER 51.2f;

float GameUnitsToMetres(float units)
{
	return units / GAME_UNITS_IN_METER;
}

float CheckUnits(float units)
{
	if (usingMetres)
		return GameUnitsToMetres(units);
	else
		return units;
}

void PrintLowInfo(vector<LevelObject*>& objs)
{
	char sep = ' ';
	for (size_t i = 0; i < objs.size(); i++)
	{
		cout << "ID: " << to_string(i) << " : {" << objs[i]->TypeName << "} "
			<< CheckUnits(objs[i]->ObjMatrix.t.x) << sep << CheckUnits(objs[i]->ObjMatrix.t.y) << sep << CheckUnits(objs[i]->ObjMatrix.t.z) << '\n';
	}
}

bool ReadAllBytes(string filePath, vector<unsigned char> &data)
{
	ifstream ifs(filePath, ios::binary | ios::ate);

	if (!ifs.is_open())
		return false;

	ifstream::pos_type pos = ifs.tellg();

	if (pos == 0)
		return false;

	ifs.seekg(0, ios::beg);

	vector<char> tempData;
	tempData.resize((size_t)pos);

	ifs.read(&tempData[0], pos);

	std::copy(tempData.begin(), tempData.end(), std::back_inserter(data));

	ifs.close();

	return true;
}

unsigned long ReadULong(vector<unsigned char> &allData, int index)
{
	unsigned long finalValue = *(unsigned long*)&allData[index];

	/*for (int i = index; i < index + 4; i++)
	{
		int power = i - index;
		finalValue += (unsigned long)(allData[i] * pow(10, power));
	}*/

	return finalValue;
}

string ReadString(vector<unsigned char>& allData, int index, int length)
{
	string out = "";

	for (int i = index; i < index + length; i++)
		out += allData[i];

	return out;
}

string MatrixToString(MatrixUnPad mat)
{
	string out = "";

	out += "Rotation matrix:\n";
	out += to_string(mat.m[0].x) + " " + to_string(mat.m[0].y) + " " + to_string(mat.m[0].z) + "\n";
	out += to_string(mat.m[1].x) + " " + to_string(mat.m[1].y) + " " + to_string(mat.m[1].z) + "\n";
	out += to_string(mat.m[2].x) + " " + to_string(mat.m[2].y) + " " + to_string(mat.m[2].z) + "\n";

	out += "Position:\n";
	out += to_string(CheckUnits(mat.t.x)) + " " + to_string(CheckUnits(mat.t.y)) + " " + to_string(CheckUnits(mat.t.z)) + "\n";

	out += "Normal: " + to_string(mat.Normal);

	return out;
}

int addons[9] = { 0x50 , 0x51 , 0x52 ,0x53, 0x54 ,0x56, 0x57, 0x58, 0x59 };
string addonNames[9] = { "ARMOUR" , "CLOAK" , "RECHARGE" ,"FARADAYCAGE", "SCAVUNIT" ,"SHIELD", "SOULUNIT", "REPAIRUNIT", "CARRIERGUNS" };
string ExtraDataToString(vector<unsigned long>& extraData)
{
	string out = "";

	if (extraData.size() == 0)
		return "\n";

	out += "Amount of addons: " + to_string(extraData.size()) + '\n';

	for (unsigned long i = 0; i < extraData.size(); i++)
		for (int j = 0; j < 9; j++)
		{
			unsigned long addonTag = extraData[i];
			if (addonTag == addons[j])
			{
				out += "    " + to_string(i) + ". " + addonNames[j] + '\n';
				break;
			}
		}

	return out;
}

void PrintInfoAboutObject(LevelObject* objectInfo)
{
	string info = "";

	info += "\nEntry: " + to_string(objectInfo->entryID) + '\n';
	info += "dwSize: " + to_string(objectInfo->dwSize) + '\n';
	info += "TypeName: " + string(objectInfo->TypeName) + '\n';
	info += "AttachName: " + string(objectInfo->AttachName) + '\n';
	info += "ObjMatrix:\n" + MatrixToString(objectInfo->ObjMatrix) + '\n';
	info += "RenderableId: " + to_string(objectInfo->RenderableId) + '\n';
	info += "ControllableId: " + to_string(objectInfo->ControllableId) + '\n';
	info += "ShadowFlags: " + to_string(objectInfo->ShadowFlags) + '\n';
	info += "Permanent flag: " + to_string(objectInfo->Permanent) + '\n';
	info += "TeamNumber: " + to_string(objectInfo->TeamNumber) + '\n';
	info += "SpecificData: " + to_string(objectInfo->SpecificData) + '\n';
	info += ExtraDataToString(objectInfo->ExtraDataSize);

	cout << info;
}

vector<LevelObject*> loadedObjects;

unsigned long interestingHalfBytes[10] = { 'CJBO', '3JBO', '3JBO', '3JBO', '3JBO', '3JBO', '3JBO', '3JBO', '3JBO', '3JBO' };
string interestingHalfBytesStr[10] = { "CJBO", "3JBO", "3JBO", "3JBO", "3JBO", "3JBO", "3JBO", "3JBO", "3JBO", "3JBO" };
bool LoadOB3File(string filePath)
{
	string fileName = GetFileName(filePath);
	//cout << "File name: " << fileName << '\n';

	vector<unsigned char> allData;
	if (!ReadAllBytes(filePath, allData))
	{
		cout << "Error finding the file: " << fileName << '\n';
		return false;
	}

	//cout << "File size in bytes: " << allData.size() << '\n';

	if (allData.size() <= 4)
		return false;

	unsigned long versionRaw = *(unsigned long*)(&allData[0]);
	string fileVersion = "";

	for (int j = 0; j < 10; j++)
		if (versionRaw == interestingHalfBytes[j])
		{
			fileVersion = interestingHalfBytesStr[j];
			reverse(fileVersion.begin(), fileVersion.end());
			//cout << "Version: " << fileVersion << '\n';
			break;
		}

	if (fileVersion == "")
	{
		cout << "No valid file version found!\n";
		return false;
	}

	if (fileVersion != "OBJC")
	{
		cout << "The file version is not OBJC, conversions are not supported!\n";
		return false;
	}

	if (allData.size() <= 8)
		return false;

	unsigned long entries = ReadULong(allData, 4); // Number of entires is stored in the file now
	
	unsigned int descriptorSize = sizeof(ObjectDescription12); // 148
	unsigned int matrixSize = sizeof(MatrixUnPad); // 52 = 4 * 12 + 1
	unsigned int vertexSize = sizeof(VertexUnPad); // 12

	int cursor = 8;

	loadedObjects.clear();
	for (unsigned long i = 0; i < entries; i++)
	{
		ObjectDescription12* ptr = (struct ObjectDescription12*)(&allData[cursor]);
		
		vector<unsigned long> ExtraDataSize;
		ExtraDataSize.resize(ptr->ExtraDataSize[0]);
		int addons = ptr->ExtraDataSize[0];
		for (int i = 0; i < addons; i++)
			ExtraDataSize[i] = ReadULong(allData, (size_t)cursor + 148 + (size_t)i * 4);

		LevelObject* newObject = new LevelObject(*ptr, ExtraDataSize);
		newObject->entryID = i;
		loadedObjects.push_back(newObject);

		cursor += ptr->dwSize; // Offset to read next entry
	}

	return true;
}

/*
 // FOR DEBUGGING
		//int tempCursor = cursor;
		//ObjectDescription12 ptr{};

		//ptr.dwSize = ReadULong(allData, tempCursor);
		//tempCursor += 4;

		//string objectName = ReadString(allData, tempCursor, 32);
		//_memccpy(ptr.TypeName, &allData[tempCursor], 0, 32);
		//tempCursor += 32;

		//string objectAttachment = ReadString(allData, tempCursor, 32);
		//_memccpy(ptr.AttachName, &allData[tempCursor], 0, 32);
		//tempCursor += 32;

		//MatrixUnPad objectMatrix = *(MatrixUnPad*)&allData[tempCursor];
		//ptr.ObjMatrix = *(MatrixUnPad*)&allData[tempCursor];
		//tempCursor += matrixSize;

*/

bool ReadFile(string path)
{
	if (path.size() < 5)
		path += ".ob3";
	else
	{
		string ending = path.substr(path.size() - 4, 4);
		if (ending != ".ob3")
			path += ".ob3";
	}

	if (!LoadOB3File(path.c_str()))
	{
		system("pause");
		return false;
	}

	return true;
}

void PrintDetailedInfo(vector<LevelObject*>& objs)
{
	for (size_t i = 0; i < objs.size(); i++)
		PrintInfoAboutObject(objs.at(i));

	system("pause");
}

void PrintDetailedInfo()
{
	PrintDetailedInfo(loadedObjects);
}

void PrintLowInfo()
{
	PrintLowInfo(loadedObjects);
}

char asciitolower(char in) {
	if (in <= 'Z' && in >= 'A')
		return in - ('Z' - 'z');
	return in;
}

void FindObject()
{
	string nameChunk;
	cout << "What is the name of the object?: ";
	cin >> nameChunk;
	cout << '\n';

	transform(nameChunk.begin(), nameChunk.end(), nameChunk.begin(), asciitolower);

	vector<LevelObject*> foundObjects;

	string s1;
	for (size_t i = 0; i < loadedObjects.size(); i++)
	{
		s1 = string(loadedObjects[i]->TypeName);

		if (nameChunk.size() > s1.size())
			continue;

		transform(s1.begin(), s1.end(), s1.begin(), asciitolower);

		if (s1.find(nameChunk) == std::string::npos)
			continue;

		foundObjects.push_back(loadedObjects[i]);
	}

	if (foundObjects.size() == 0)
	{
		cout << "No objects found... :(\n";
		system("pause");
		return;
	}

	cout << "Found " << to_string(foundObjects.size()) << " objects!\n";
	PrintDetailedInfo(foundObjects);
}

bool needsToSave = false;
void SaveFile()
{
	ofstream write("level1.ob3", ios::binary);
	
	if (!write.is_open())
	{
		cout << "Failed to create a save file?\n";
		system("pause");
		return;
	}

	write << "OBJC";

	unsigned long entries = (unsigned long)loadedObjects.size();
	char* tempData = (char*)&entries;
	write.write(tempData, sizeof(unsigned long));

	ObjectDescription12 dummyDesc{};
	for (size_t i = 0; i < loadedObjects.size(); i++)
	{
		LevelObject* ptr = loadedObjects[i];

		dummyDesc.dwSize = ptr->dwSize;

		for (int j = 0; j < 32; j++)
		{
			dummyDesc.TypeName[j] = ptr->TypeName[j];
			dummyDesc.AttachName[j] = ptr->AttachName[j];
		}

		dummyDesc.ObjMatrix = ptr->ObjMatrix;

		dummyDesc.RenderableId = ptr->RenderableId;
		dummyDesc.ControllableId = ptr->ControllableId;
		dummyDesc.ShadowFlags = ptr->ShadowFlags;
		dummyDesc.Permanent = ptr->Permanent;
		dummyDesc.TeamNumber = ptr->TeamNumber;
		dummyDesc.SpecificData = ptr->SpecificData;
		dummyDesc.ExtraDataSize[0] = (unsigned long)ptr->ExtraDataSize.size();

		tempData = (char*)&dummyDesc;
		write.write(tempData, sizeof(dummyDesc));

		if (ptr->ExtraDataSize.size() == 0)
			continue;

		for (unsigned long j = 0; j < ptr->ExtraDataSize.size(); j++)
		{
			unsigned long extra = ptr->ExtraDataSize[j];

			tempData = (char*)&extra;
			write.write(tempData, sizeof(unsigned long));
		}
	}

	write.close();

	needsToSave = false;

	cout << "Done!\n";
	system("pause");
}

int GetTypeChoice(string promptText, vector<string> &printStuff)
{
	string out = "";

	for (size_t i = 0; i < printStuff.size(); i++)
		out += to_string(i) + ". " + printStuff[i] + "\n";

	out += promptText;
	cout << out;

	int type = -1;
	cin >> type;

	return type;
}

int GetTypeChoicePair(string promptText, vector<pair<string, unsigned long>>& printStuff)
{
	string out = "";

	for (size_t i = 0; i < printStuff.size(); i++)
		out += to_string(i) + ". " + printStuff[i].first + "\n";

	out += promptText;
	cout << out;

	int type = -1;
	cin >> type;

	return type;
}

string GetTypeName()
{
startAgain:;

	system("cls");

	int type = GetTypeChoice("Choose an object type, type -1 for custom type\n", knownObjs);

	string typeName = "";
	if (type == -1)
	{
		cout << "ok smartypants, what is the object type name?\n";
		cin >> typeName;
	}
	else
	{
		if (type > knownObjs.size() || type < 0)
		{
			cout << "Hey it's out of range! What are you doing?? >:(\n";
			system("pause");
			goto startAgain;
		}

		typeName = knownObjs[type];
	}

	return typeName;
}

string GetAttachName()
{
startAgainB:;

	string attachName = "";
	int type = GetTypeChoice("Choose an weapon attachment type, type -1 to skip, -2 for custom type, \n", knownWeapons);

	if (type == -2)
	{
		cout << "Ok smartypants, what is the object type name?\n";
		cin >> attachName;
	}
	else if (type != -1)
	{
		if (type > knownWeapons.size() || type < 0)
		{
			cout << "Hey it's out of range! What are you doing?? >:(\n";
			system("pause");
			goto startAgainB;
		}

		attachName = knownWeapons[type];
	}

	return attachName;
}

VertexUnPad GetPosition()
{
	float readF;
	VertexUnPad pos{};

	if (usingMetres)
	{
		cout << "What should the position be for this unit? (in meters)\nx: ";
		cin >> readF;
		pos.x = readF * GAME_UNITS_IN_METER;

		cout << "y: ";
		cin >> readF;
		pos.y = readF * GAME_UNITS_IN_METER;

		cout << "z: ";
		cin >> readF;
		pos.z = readF * GAME_UNITS_IN_METER;
	}
	else
	{
		cout << "What should the position be for this unit? (in \"game units\" (NOT METERS >:( ))\nx: ";
		cin >> readF;
		pos.x = readF;

		cout << "y: ";
		cin >> readF;
		pos.y = readF;

		cout << "z: ";
		cin >> readF;
		pos.z = readF;
	}

	return pos;
}

unsigned long GetTeamNumber()
{
	unsigned long teamNumber = 0; // 0 - player

	cout << "Enter the team number (0 - player, 1 - probably enemy...)\n";
	cin >> teamNumber;

	return teamNumber;
}

int AddAddon(LevelObject* newObj)
{
	int type = GetTypeChoicePair("Select an addon you want to add. (-1 to stop, -2 to enter custom code)\n", knownAddons);

	if (type == -2)
	{
		cout << "Ok hackerman, what's the secret code for this addon? (Should be from 80 to 89 I think)\n";
		cin >> type;

		newObj->ExtraDataSize.push_back(type);
		newObj->dwSize += 4; // More data than expected, needs to be bigger
	}
	else if (type != -1)
	{
		if (type >= knownAddons.size() || type < 0)
		{
			cout << "Selection out of range!\n";
			system("pause");
			return false;
		}

		unsigned long ul = knownAddons[type].second; // XX 00 00 00
		if (ul == 87) // It's a soulcatcher, add a soulname in a hacky way... these damn devs
		{
			unsigned long choice = GetTypeChoicePair("Choose the soul for your soulcatcher\n", knownSouls);

			choice = knownSouls[choice].second << 16; // YY 00 00 00 -> 00 00 YY 00
			ul = ul | choice; // XX 00 YY 00
		}

		newObj->ExtraDataSize.push_back(ul);
		newObj->dwSize += 4; // More data than expected, needs to be bigger
	}

	return type == -1;
}

void ReplaceAddon(LevelObject* newObj, int where)
{
	int type = GetTypeChoicePair("Select an addon you want to use for replacement. (-1 to enter custom code)\n", knownAddons);

	if (type == -1)
	{
		cout << "Ok hackerman, what's the secret code for this addon? (Should be from 80 to 89 I think)\n";
		cin >> type;

		newObj->ExtraDataSize[where] = type;
	}
	else if (type > 0)
	{
		if (type >= knownAddons.size())
		{
			cout << "Selection out of range!\n";
			system("pause");
			return;
		}

		unsigned long ul = knownAddons[type].second; // XX 00 00 00
		if (ul == 87) // It's a soulcatcher, add a soulname in a hacky way... these damn devs
		{
			unsigned long choice = GetTypeChoicePair("Choose the soul for your soulcatcher\n", knownSouls);

			choice = knownSouls[choice].second << 16; // YY 00 00 00 -> 00 00 YY 00
			ul = ul | choice; // XX 00 YY 00
		}

		newObj->ExtraDataSize[where] = ul;
	}
}

void AddObject()
{
	string typeName = GetTypeName();
	string attachName = GetAttachName();
	VertexUnPad pos = GetPosition();

	unsigned long teamNumber = GetTeamNumber();

	LevelObject* newObj = new LevelObject();

	newObj->dwSize = sizeof(ObjectDescription12);

	newObj->SetTypeName(typeName);
	newObj->SetAttachName(attachName);

	newObj->TeamNumber = teamNumber;

	unordered_set<unsigned long> loadedIds;
	for (size_t i = 0; i < loadedObjects.size(); i++)
		loadedIds.insert(loadedObjects[i]->RenderableId);

	unsigned long unusedRendId = -1;
	for (unsigned long i = 1; i < loadedObjects.size() + 10; i++)
		if (loadedIds.count(i) == 0)
		{
			unusedRendId = i;
			break;
		}

	if (unusedRendId == -1)
		unusedRendId = rand() % 1000 + loadedObjects.size() + 10;

	newObj->RenderableId = unusedRendId;
	newObj->entryID = loadedObjects.size();

	newObj->ObjMatrix.t = pos;

	string answer = "";
	cout << "Do you want any addons(components/modules)? Y/N\n";
	cin >> answer;

	if (answer == "Y" || answer == "y")
		while (!AddAddon(newObj));

	loadedObjects.push_back(newObj);

	needsToSave = true;

	cout << "Object added!\n";
	system("pause");
}

void RemoveObject()
{
	PrintLowInfo();

again:;

	int id;
	cout << "Which object do you want to remove?\n";
	cin >> id;

	if (id < 0 || id >= loadedObjects.size())
	{
		cout << "ID is out of bounds!\n";
		system("pause");
		goto again;
	}

	string name = loadedObjects[id]->TypeName;

	delete loadedObjects[id];
	loadedObjects.erase(loadedObjects.begin() + id);

	for (int i = id; i < loadedObjects.size(); i++)
		loadedObjects[i]->entryID = i;

	needsToSave = true;

	cout << id << " " << name << " removed!\n";
	system("pause");
}

float Magnitude(VertexUnPad foo)
{
	return sqrt(foo.x * foo.x + foo.y * foo.y + foo.z * foo.z);
}

float Scalar(VertexUnPad foo, VertexUnPad bar)
{
	return bar.x * foo.x + bar.y * foo.y + bar.z * foo.z;
}

float VectorAngle(VertexUnPad foo, VertexUnPad bar) // IN RADIANS
{
	return acos(
		Scalar(foo, bar)
		/
		(Magnitude(foo) * Magnitude(bar))
	);
}

#define PI 3.14159265358979323846  /* pi */
const static float Deg2Rad = (PI * 2.) / 360.;
const static float Rad2Deg = 360. / (PI * 2.);

#define E 0.00001

bool EpsilonFloatEquals(float f, float to)
{
	return f > to - E && f < to + E;
}

VertexUnPad MatrixToEulerAngles(VertexUnPad m[3])
{
	/*eulers.x = atan2(m[2].y, m[2].z) * Rad2Deg;
	eulers.y = atan2(-m[2].x, sqrt(m[2].y * m[2].y + m[2].z * m[2].z)) * Rad2Deg;
	eulers.z = atan2(m[1].x, m[0].x) * Rad2Deg;*/

	float R11 = m[0].x;
	float R12 = m[0].y;
	float R13 = m[0].z;

	float R21 = m[1].x;
	float R23 = m[1].z;

	float R31 = m[2].x;
	float R32 = m[2].y;
	float R33 = m[2].z;

	float E1, E2, E3, delta;
	if (EpsilonFloatEquals(R13, 1) || EpsilonFloatEquals(R13, -1))
	{
		E3 = 0;
		delta = atan2(R12, R13);

		if (EpsilonFloatEquals(R13, -1))
		{
			E2 = PI / 2;
			E1 = E3 + delta;
		}
		else
		{
			E2 = -PI / 2;
			E1 = -E3 + delta;
		}
	}
	else
	{
		E2 = -asin(R13);
		E1 = atan2(R23 / cos(E2), R33 / cos(E2));
		E3 = atan2(R12 / cos(E2), R11 / cos(E2));
	}

	VertexUnPad eulers = VertexUnPad(E1, E2, E3);

	eulers.x *= Rad2Deg;
	eulers.y *= Rad2Deg;
	eulers.z *= Rad2Deg;

	return eulers;
}

string EulerAnglesToString(VertexUnPad eulers)
{
	return "X:" + to_string(eulers.x) + "  Y:" + to_string(eulers.y) + "  Z:" + to_string(eulers.z);
}



void RotateByX(VertexUnPad& vec, float alpha)
{
	float Y = vec.y;
	float Z = vec.z;

	vec.y = Y * cos(alpha) - Z * sin(alpha);
	vec.z = Y * sin(alpha) + Z * cos(alpha);
}

void RotateByY(VertexUnPad& vert, float alpha)
{
	float X = vert.x;
	float Z = vert.z;

	vert.x = X * cos(alpha) + Z * sin(alpha);
	vert.z = -X * sin(alpha) + Z * cos(alpha);
}

void RotateByZ(VertexUnPad& vec, float alpha)
{
	float X = vec.x;
	float Y = vec.y;

	vec.x = X * cos(alpha) - Y * sin(alpha);
	vec.y = X * sin(alpha) + Y * cos(alpha);
}

void RotateMatByVector(MatrixUnPad &objMat, VertexUnPad euler)
{
	RotateByX(objMat.m[0], euler.x);
	RotateByX(objMat.m[1], euler.x);
	RotateByX(objMat.m[2], euler.x);

	RotateByY(objMat.m[0], euler.y);
	RotateByY(objMat.m[1], euler.y);
	RotateByY(objMat.m[2], euler.y);

	RotateByZ(objMat.m[0], euler.z);
	RotateByZ(objMat.m[1], euler.z);
	RotateByZ(objMat.m[2], euler.z);
}

void EditObject()
{
	PrintLowInfo();

again:;

	int id;
	cout << "Which object do you want to edit?\n";
	cin >> id;

	if (id < 0 || id >= loadedObjects.size())
	{
		cout << "ID is out of bounds!\n";
		system("pause");
		goto again;
	}

	int choice;
	do
	{
		system("cls");
		cout << "Currently editing: {" << loadedObjects[id]->TypeName << "}\n"
			<< "What do you want to change?"
			<< "\n1. Type name: " << loadedObjects[id]->TypeName
			<< "\n2. Main weapon: " << loadedObjects[id]->AttachName
			<< "\n3. Position: " << CheckUnits(loadedObjects[id]->ObjMatrix.t.x) << " " << CheckUnits(loadedObjects[id]->ObjMatrix.t.y) << " " << CheckUnits(loadedObjects[id]->ObjMatrix.t.z)
			<< "\n4. Rotation: " << EulerAnglesToString(MatrixToEulerAngles(loadedObjects[id]->ObjMatrix.m)) << " degrees"
			<< "\n5. Normal flag: " << to_string(loadedObjects[id]->ObjMatrix.Normal)
			<< "\n6. Renderable ID: " << to_string(loadedObjects[id]->RenderableId)
			<< "\n7. Controllable ID: " << to_string(loadedObjects[id]->ControllableId)
			<< "\n8. ShadowFlags Code: " << to_string(loadedObjects[id]->ShadowFlags)
			<< "\n9. Permanent flag: " << to_string(loadedObjects[id]->Permanent)
			<< "\n10. Team number: " << to_string(loadedObjects[id]->TeamNumber)
			<< "\n11. Specific data code: " << to_string(loadedObjects[id]->SpecificData)
			<< "\n12. Change addons: " << ExtraDataToString(loadedObjects[id]->ExtraDataSize)
			<< "13. Stop\n";

		cin >> choice;

		if (choice < 1 || choice > 12)
			break;

		unsigned int tmp_u_int;
		VertexUnPad angles;
		switch (choice)
		{
		case 1:
			loadedObjects[id]->SetTypeName(GetTypeName());
			needsToSave = true;
			break;

		case 2:
			loadedObjects[id]->SetAttachName(GetAttachName());
			needsToSave = true;
			break;

		case 3:
			loadedObjects[id]->ObjMatrix.t = GetPosition();
			needsToSave = true;
			break;

		case 4:
			cout << "What angle do you want your object to look at? from -180 to 180\nx(up/down):";
			cin >> angles.x;
			cout << "y(left/right/around):";
			cin >> angles.y;
			cout << "z(forward swirl):";
			cin >> angles.z;

			angles.x *= Deg2Rad;
			angles.y *= Deg2Rad;
			angles.z *= Deg2Rad;

			loadedObjects[id]->ResetRotation();
			RotateMatByVector(loadedObjects[id]->ObjMatrix, angles);
			needsToSave = true;
			break;

		case 5:
			cout << "Enter normal flag 0/1\n";
			cin >> loadedObjects[id]->ObjMatrix.Normal;
			needsToSave = true;
			break;

		case 6:
			cout << "Enter Renderable ID\n";
			cin >> loadedObjects[id]->RenderableId;
			needsToSave = true;
			break;

		case 7:
			cout << "Enter Controllable ID\n";
			cin >> loadedObjects[id]->ControllableId;
			needsToSave = true;
			break;

		case 8:
			cout << "Enter ShadowFlags Code\n";
			cin >> loadedObjects[id]->ShadowFlags;
			needsToSave = true;
			break;

		case 9:
			cout << "Enter permanent flag (0-255)\n";
			cin >> tmp_u_int;
			loadedObjects[id]->Permanent = (char)tmp_u_int;
			needsToSave = true;
			break;

		case 10:
			cout << "Enter team number\n";
			cin >> loadedObjects[id]->TeamNumber;
			needsToSave = true;
			break;

		case 11:
			cout << "Specific data code\n";
			cin >> loadedObjects[id]->SpecificData;
			needsToSave = true;
			break;

		case 12:

			chooseAgain:;
			system("cls");

			cout << ExtraDataToString(loadedObjects[id]->ExtraDataSize) << "Do you want to:\n1. Add an addon\n2. Remove it\n3. Replace\n4. Stop\n";
			cin >> choice;

			if (choice == 1)
				AddAddon(loadedObjects[id]);
			else if (choice == 2)
			{
				cout << "Choose an addon to remove:\n" << ExtraDataToString(loadedObjects[id]->ExtraDataSize);
				cin >> choice;

				if (choice < 0 || choice >= loadedObjects[id]->ExtraDataSize.size())
				{
					cout << "Choice is out of range!\n";
					system("pause");
					goto chooseAgain;
				}

				loadedObjects[id]->ExtraDataSize.erase(loadedObjects[id]->ExtraDataSize.begin() + choice);
				loadedObjects[id]->dwSize -= 4;
			}
			else if (choice == 3)
			{
				cout << "Choose an addon to replace:\n" << ExtraDataToString(loadedObjects[id]->ExtraDataSize);
				cin >> choice;

				if (choice < 0 || choice >= loadedObjects[id]->ExtraDataSize.size())
				{
					cout << "Choice is out of range!\n";
					system("pause");
					goto chooseAgain;
				}

				ReplaceAddon(loadedObjects[id], choice);
			}
			else
				break;

			needsToSave = true;
			goto chooseAgain;
			break;

		default:
			break;
		}

		if (needsToSave)
		{
			cout << "Changes saved in memory\n";
			system("pause");
		}

	} while (true);

}

void PrintCredits()
{
	system("cls");
	cout << "Made by Game Dream St.\nJoin our Hostile Waters Community Discord server! https://discord.gg/sqsKRw4 \n";
	system("pause");
}

string MetresOn()
{
	if (usingMetres)
		return "ON";
	else
		return "OFF";
}

void ToggleMetres()
{
	usingMetres = !usingMetres;
}

int GetRandomInt(int from, int to) // from - inclusive. to - exclusive
{
	return rand() % (to - from) + from;
}

void RandomizeAllUnits()
{
	unordered_map<string, vector<LevelObject*>> differentUnits;

	for (size_t i = 0; i < loadedObjects.size(); i++)
		differentUnits[loadedObjects[i]->TypeName].push_back(loadedObjects[i]);

	unordered_map<string, vector<LevelObject*>>::iterator it = differentUnits.begin();

	vector<string> choiceObjs = knownObjs;

	while (it != differentUnits.end() && choiceObjs.size() > 0)
	{
		int rnd = GetRandomInt(0, choiceObjs.size());
		string randomType = choiceObjs[rnd];
		choiceObjs.erase(choiceObjs.begin() + rnd);

		vector<LevelObject*> objects = (*it).second;

		for (size_t i = 0; i < objects.size(); i++)
		{
			objects[i]->SetTypeName(randomType);
			objects[i]->TeamNumber = 1;
		}

		it++;
	}

	needsToSave = true;

	cout << "Done!\n";
	system("pause");
}

void RandomizeOneUnit()
{
	unordered_map<string, vector<LevelObject*>> differentUnits;

	for (size_t i = 0; i < loadedObjects.size(); i++)
		differentUnits[loadedObjects[i]->TypeName].push_back(loadedObjects[i]);

	unordered_map<string, vector<LevelObject*>>::iterator it = differentUnits.begin();

	int itRnd = GetRandomInt(0, differentUnits.size());
	for (size_t i = 0; i < itRnd; i++)
		it++;

	vector<LevelObject*> objects = (*it).second;

	string from = "";
	if (objects.size() > 0)
		from = objects[0]->TypeName;

	string randomType;
	bool notOk = true;
	do
	{
		int rnd = GetRandomInt(0, knownObjs.size());
		randomType = knownObjs[rnd];

		cout << "From " << from << " To: " << randomType << '\n'
		<< "Is that ok? Y/N (S = stop)\n";

		string answer;
		cin >> answer;

		if (answer == "y" || answer == "Y")
			notOk = false;
		else if (answer == "s" || answer == "S")
			return;
	} while (notOk);


	for (size_t i = 0; i < objects.size(); i++)
	{
		objects[i]->SetTypeName(randomType);
		objects[i]->TeamNumber = 1;
	}

	needsToSave = true;

	cout << "Done!\n";
	system("pause");
}

void Ranzomizer()
{
	do
	{
		system("cls");
		cout << "Randomizer:\n"
			//<< "1. Randomize EVERYTHING\n"
			<< "1. Randomize all units\n"
			<< "2. Randomize one unit\n"
			//<< "3. Randomize a specific unit\n"
			//<< "4. Randomize unlock\ns"
			//<< "5. Randomize EJ\n"
			//<< "6. Randomize carrier shells\n"
			//<< "7. Randomize enemy waves\n"
			//<< "8. Randomize enemy bases\n"
			<< "3. Back\n";

		int choice = -1;
		cin >> choice;

		if (choice == 1)
			RandomizeAllUnits();
		else if (choice == 2)
			RandomizeOneUnit();
		else
			break;

	} while (true);
}

int main(int argc, char* argv[])
{
	srand(time(NULL));

	string path = "";

	// Command line
	if (argc > 1)
		path = argv[1];

	bool close = true, editing = true;
	do
	{
		close = true;

		if (path == "")
		{
			typeFilename:;

			printf("What .ob3 file do you want to read?\n(Enter absolute or relative path. You can also drag and drop the file)\n");

			string fileName;
			getline(std::cin, fileName);

			fileName = RemoveQuotes(fileName);

			if (!IsAPath(fileName))
				path = RemovePathLast(string(argv[0])) + "\\" + fileName;
			else
				path = fileName;
		}
		else
			path = RemoveQuotes(path);

		if (!ReadFile(path))
		{
			system("cls");
			goto typeFilename;
		}

		string file = GetFileName(path);

		do
		{
			editing = true;
			system("cls");

			if (needsToSave)
				cout << "Don't forget to save!\n\n";

			cout << "Curently editing " << file << '\n'
				<< "What would you like to do?\n"
				<< "1. List all objects\n"
				<< "2. List all objects in detail\n"
				<< "3. Find object\n"
				<< "4. Add object\n"
				<< "5. Edit object\n"
				<< "6. Remove object\n"
				<< "7. Save file\n"
				<< "8. Use meters instead of game units ["<< MetresOn() <<"]\n"
				<< "9. Randomizer\n"
				<< "10. Credits\n"
				<< "11. Quit\n";

			int choice = -1;
			cin >> choice;

			if (choice == 1)
			{
				PrintLowInfo();
				system("pause");
			}
			else if (choice == 2)
				PrintDetailedInfo();
			else if (choice == 3)
				FindObject();
			else if (choice == 4)
				AddObject();
			else if (choice == 5)
				EditObject();
			else if (choice == 6)
				RemoveObject();
			else if (choice == 7)
				SaveFile();
			else if (choice == 8)
				ToggleMetres();
			else if (choice == 9)
				Ranzomizer();
			else if (choice == 10)
				PrintCredits();
			else
				editing = false;

		} while (editing);

		needsToSave = false;

		printf("Do you wish to continue? Y/N: ");
		cin >> path;
		printf("\n");

		if (path == "Y" || path == "y")
			close = false;

		path = "";
	} while (!close);

	return 0;
}
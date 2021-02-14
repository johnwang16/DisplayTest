#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>
#include <ntddvdeo.h>

#pragma comment(lib, "Kernel32.lib")

#define STATUS_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\default$windows.data.bluelightreduction.bluelightreductionstate\\windows.data.bluelightreduction.bluelightreductionstate"
#define STATE_DATA_LENGTH 44
#define STATE_VALUE_NAME "Data"

bool ModifyBrightness(bool nightLightIsOn);
bool SetDisplayBrightness(HANDLE hDevice, DISPLAY_BRIGHTNESS* displayBrightness);
bool GetNightLightStateData(unsigned char * byteArray, DWORD size);
bool ProcessNightLightStateData(unsigned char* byteArray, DWORD size);
bool WriteDataToRegistry(unsigned char* byteArray, DWORD size, bool nightLightState);

int main(int argc, char *argv[]) {

	DWORD size = STATE_DATA_LENGTH;
	unsigned char* regVal = (unsigned char*)malloc(size * sizeof(unsigned char));
	bool nightLightIsOn = false;
	
	if (GetNightLightStateData(regVal, size)) {
		nightLightIsOn = ProcessNightLightStateData(regVal, size);
		if (argc > 1){
			if (_stricmp(argv[1], "on") == 0 && !nightLightIsOn) {
				//ModifyBrightness(nightLightIsOn);
				WriteDataToRegistry(regVal, size, nightLightIsOn);
			}
			if (_stricmp(argv[1], "off") == 0 && nightLightIsOn) {
				//ModifyBrightness(nightLightIsOn);
				WriteDataToRegistry(regVal, size, nightLightIsOn);
			}
		}
		else {
			printf("Night Light on is %d\n", nightLightIsOn);
			//ModifyBrightness(nightLightIsOn);
			//WriteDataToRegistry(regVal, size, nightLightIsOn);
		}
	}	

	free(regVal);

	return 0;
}

bool WriteDataToRegistry(unsigned char* byteArray, DWORD size, bool nightLightState) {

	bool retval = false;
	HKEY hKey = HKEY_CURRENT_USER;
	LPCSTR subKey = STATUS_PATH;
	LONG err;
	int baseSize = 42; //was 41

	err = RegOpenKeyEx(hKey, subKey, 0, KEY_SET_VALUE, &hKey);

	if (nightLightState)
		size = baseSize;
	else
		size = baseSize+2;

	if (err != ERROR_SUCCESS)
		printf("The %s subkey could not be opened. Error code: %x\n", subKey, err);
	else {
		err = RegSetValueEx(hKey, "Data", 0, REG_BINARY, byteArray, size);

		if (err != ERROR_SUCCESS)
			printf("Error getting value. Code: %x\n", err);
		else
			retval = true;

		RegCloseKey(hKey);
	}

	return retval;
}

bool ProcessNightLightStateData(unsigned char* byteArray, DWORD size) {
	int indicatorIndex = 19; //was 18 prior to 2021-02-11
	int insertIndex = 24; //was 23
	int incrementIndex = 10;
	bool nightLightIsOn = false;
	int ch = byteArray[indicatorIndex];

	if (ch == 0x15) {

		nightLightIsOn = true;

		for (int i = incrementIndex; i < incrementIndex + 5; i++) {
			ch = byteArray[i];
			if (ch != 0xff) {
				byteArray[i]++;
				break;
			}
		}
		
		byteArray[indicatorIndex] = 0x13;

		for (int i = insertIndex+1; i >= insertIndex; i--)
			for (int j = i; j < size - 2; j++)
				byteArray[j] = byteArray[j + 1];
	}
	else if (ch == 0x13) {

		nightLightIsOn = false;

		for (int i = incrementIndex; i < incrementIndex+5; i++) {
			ch = byteArray[i];
			if (ch != 0xff) {
				byteArray[i]++;
				break;
			}
		}

		byteArray[indicatorIndex] = 0x15;
		
		int n = 0;
		while (n < 2) {
			for (int j = size - 1; j > insertIndex; j--)
				byteArray[j] = byteArray[j - 1];
			n++;
		}
	
		byteArray[insertIndex] = 0x10;
		byteArray[insertIndex+1] = 0x00;
	}

	return nightLightIsOn;
}

bool GetNightLightStateData(unsigned char* byteArray, DWORD size) {

	bool retval = false;
	HKEY hKey = HKEY_CURRENT_USER;
	LPCSTR subKey = STATUS_PATH, pValue = STATE_VALUE_NAME;
	DWORD dataType = 0;
	LONG err;

	err = RegOpenKeyEx(hKey, subKey, 0, KEY_READ, &hKey);

	if (err != ERROR_SUCCESS)
		printf("The %s subkey could not be opened. Error code: %x\n", subKey, err);
	else {
		err = RegGetValue(hKey, NULL, pValue, RRF_RT_REG_BINARY, &dataType, byteArray, &size);

		if (err != ERROR_SUCCESS)
			printf("Error getting value. Code: %x\n", err);
		else
			retval = true;

		RegCloseKey(hKey);
	}

	return retval;
}

bool ModifyBrightness(bool nightLightIsOn) {

	bool retval = false;
	DISPLAY_BRIGHTNESS displayBrightness;
	HANDLE hDevice = CreateFile("\\\\.\\LCD", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	displayBrightness.ucDisplayPolicy = 0;
	displayBrightness.ucACBrightness = 0;
	displayBrightness.ucDCBrightness = 0;

	if (hDevice != INVALID_HANDLE_VALUE) {
		if (nightLightIsOn) {
			// Asja Computer
			displayBrightness.ucDCBrightness = 80;
			displayBrightness.ucACBrightness = 80;

			/* ILJAS Computer
			displayBrightness.ucDCBrightness = 100;
			displayBrightness.ucACBrightness = 100;
			*/

			SetDisplayBrightness(hDevice, &displayBrightness);
		}
		else {
			// Asja Computer
			displayBrightness.ucDCBrightness = 0;
			displayBrightness.ucACBrightness = 0;

			/* ILJAS Computer
			displayBrightness.ucDCBrightness = 80;
			displayBrightness.ucACBrightness = 80;
			*/

			SetDisplayBrightness(hDevice, &displayBrightness);
		}
		retval = true;
	}
	else
		printf("CreateFiles failed with error %d\n", GetLastError());

	return retval;
}

bool GetDisplayBrightness(HANDLE hDevice, DISPLAY_BRIGHTNESS* displayBrightness) {

	DWORD ret = 0;

	return DeviceIoControl(hDevice, IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS, NULL, 0, displayBrightness, sizeof(*displayBrightness), &ret, NULL);
}

bool SetDisplayBrightness(HANDLE hDevice, DISPLAY_BRIGHTNESS * displayBrightness) {

	DWORD ret = 0;
	
	return DeviceIoControl(hDevice, IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS, displayBrightness, sizeof(*displayBrightness), NULL, 0, &ret, NULL);
}

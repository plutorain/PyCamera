#include <Python.h>

// https://msdn.microsoft.com/en-us/library/windows/desktop/dd377566(v=vs.85).aspx
#include <windows.h>
#include <dshow.h>
#include <comutil.h>

#pragma comment(lib, "strmiids")
// #pragma comment(lib, "kernel32")
// #pragma comment(lib, "user32")
// #pragma comment(lib, "gdi32")
// #pragma comment(lib, "winspool")
// #pragma comment(lib, "comdlg32")
// #pragma comment(lib, "advapi32")
// #pragma comment(lib, "shell32")
// #pragma comment(lib, "ole32")
// #pragma comment(lib, "oleaut32")
// #pragma comment(lib, "uuid")
// #pragma comment(lib, "odbc32")
// #pragma comment(lib, "odbccp32")
#pragma comment(lib, "comsuppwd.lib")

HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
	// Create the System Device Enumerator.
	ICreateDevEnum *pDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the category.
		hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
		if (hr == S_FALSE)
		{
			hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
		}
		pDevEnum->Release();
	}
	return hr;
}

PyObject* DisplayDeviceInformation(IEnumMoniker *pEnum)
{
	// Create an empty Python list
	PyObject* pyList = PyList_New(0); 

	IMoniker *pMoniker = NULL;

	while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
	{
		IPropertyBag *pPropBag;
		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;
		}

		VARIANT var;
		VariantInit(&var);

		// Get description or friendly name.
		hr = pPropBag->Read(L"Description", &var, 0);
		if (FAILED(hr))
		{
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
		}
		if (SUCCEEDED(hr))
		{
			// Append a result to Python list
			char  *pValue = _com_util::ConvertBSTRToString(var.bstrVal);
			hr = PyList_Append(pyList, Py_BuildValue("s", pValue));
			delete[] pValue;  
			if (FAILED(hr)) {
				printf("Failed to append the object item at the end of list list\n");
				return pyList;
			}

			// printf("%S\n", var.bstrVal);
			VariantClear(&var);
		}

		hr = pPropBag->Write(L"FriendlyName", &var);

		// WaveInID applies only to audio capture devices.
		hr = pPropBag->Read(L"WaveInID", &var, 0);
		if (SUCCEEDED(hr))
		{
			printf("WaveIn ID: %d\n", var.lVal);
			VariantClear(&var);
		}

		hr = pPropBag->Read(L"DevicePath", &var, 0);
		if (SUCCEEDED(hr))
		{
			// The device path is not intended for display.
			// printf("Device path: %S\n", var.bstrVal);
			VariantClear(&var);
		}

		pPropBag->Release();
		pMoniker->Release();
	}

	return pyList;
}

static PyObject *
getDeviceList(PyObject *self, PyObject *args)
{
	PyObject* pyList = NULL; 
	printf("getDeviceList!!! C++ API\n");
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	switch(hr)
	{
		case S_OK: printf("S_OK: The COM library was initialized successfully on this thread.\n"); break;
		case S_FALSE: printf("S_FALSE : The COM library is already initialized on this thread.\n"); break;
		case RPC_E_CHANGED_MODE: printf("RPC_E_CHANGED_MODE : A previous call to CoInitializeEx specified the concurrency model for this thread as multithread apartment (MTA). This could also indicate that a change from neutral-threaded apartment to single-threaded apartment has occurred.\n"); break;
	}
	if (SUCCEEDED(hr))
	{
		IEnumMoniker *pEnum;
		hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
		if (SUCCEEDED(hr))
		{
			pyList = DisplayDeviceInformation(pEnum);
			pEnum->Release();
		}
		CoUninitialize();
	}
    return pyList;
}

static PyMethodDef Methods[] =
{
    {"getDeviceList", getDeviceList, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initdevice(void)
{
     (void) Py_InitModule("device", Methods);
}

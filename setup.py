from distutils.core import setup, Extension

module_device = Extension('device',
                        sources = ['device.cpp'], 
                        library_dirs=['C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib']
                      )

setup (name = 'WindowsDevices',
        version = '1.0',
        description = 'Get device list with DirectShow',
        ext_modules = [module_device])

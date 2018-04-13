from distutils.core import setup, Extension

import spxdrvversion

module1 = Extension('spxmodule',
                    libraries = ['spx', 'usb'],
                    library_dirs = ['.'],
                    sources = ['spxmodule.c'])

                   # define_macros = [('MAJOR_VERSION', '1'),
                   #                  ('MINOR_VERSION', '0')],
                   # extra_compile_args =['-g'],
                   
module2 = Extension('ccsmodule',
                    libraries = ['ccs', 'usb'],
                    library_dirs = ['.'],
                    sources = ['ccsmodule.c'])

setup (name = 'thorspec',
       version = spxdrvversion.VERSION,
       description = 'Driver for Thorlabs SPx-USB spectrometers',
       author = 'Mark Colclough',
       packages = ['thorspec'],
       ext_package='thorspec',
       ext_modules = [module1, module2])


#python setup.py -v  build -f   to see the build fully

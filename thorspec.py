#!/usr/bin/python
#
# Thorlabs SP1-USB fibre spectrometer control
# Also works with CCS series, autodetected.
#
# TODO separate startscan and getscandata calls, so GUI response is
# not tied to device integration time.  No need to fix until we want to make use of
# much longer int times available on ccs device. Would need a way of showing
# that data were awaited as well.

import os, getpass, time, sys, numpy
from fltk import *
import thorspecUI as ui
from thorspecversion import VERSION
import spx  # After successful init, one of these modules is assigned to device
import ccs  #
import multiplot

UPDATETIME = 0.3 # seconds between updates.
SPOOLER = '|lpr'
VIEWER = 'evince'

dataline = None  # Keeps Ca_Line alive

# An identifier for the plots
# os.getlogin() does not work when launched from desktop - hence getpass
def usertime():
        return ''.join([getpass.getuser(), ' at ', os.uname()[1], ' -- ',
                                    time.strftime('%H:%M %a %d %B %Y')] )
                                    
#================== Main menu =========================
def menu_cb(w):
        global wavelengths, intensities
        choice = w.mvalue().label()
        if choice == 'Save':
                newname = fl_file_chooser('Save data as...', # title
                                          'Text files (*.txt)\tAll files (*)', # pattern
                                          '\x00')            # name
                if newname is None:
                        return
                root, ext = os.path.splitext(newname)
                if ext == '':
                        newname = root + '.txt'
                if os.path.exists(newname):
                        fl_alert("Can't write file %s\n - It already exists!"%newname)
                        return
                try:
                        f = file(newname, 'w')
                except:
                        fl_alert('Unable to write file %s\n (permissions? disk full?)!'%newname)
                        return
                for point in zip(wavelengths, intensities)[LOWDAT:DATLIMIT]:
                        f.write('%f\t%f\n'%(point[0], point[1]))
                f.close()

        elif choice == 'Load':
                filname = fl_file_chooser('Select file to load', # title
                                          'Text files (*.txt)\tAll files (*)', # pattern
                                          '\x00')            # name
                if filname is None:
                        return
                try:
                        f = file(filname, 'r')
                except:
                        fl_alert('Unable to read file %s\n!'%filname)
                        return
                line = 'first_data_line'   # initial contents for error msg
                ui.runbut.value(0)  # stop loaded data being overwritten
                
                # fill with zeros where we did not save data and are not using results
                wavelengths[:LOWDAT] = 0.0
                wavelengths[DATLIMIT:] = 0.0
                intensities[:LOWDAT] = 0.0
                intensities[DATLIMIT:] = 0.0
                datcount = LOWDAT
                try:
                        for line in f:
                                if line.strip() != '':
                                        strs = line.split()
                                        xin = float(strs[0])
                                        yin = float(strs[1])
                                        wavelengths[datcount] = xin
                                        intensities[datcount] = yin
                                        datcount = datcount + 1
                                        if datcount == DATLIMIT: break
                        auto_scale()
                except:
                        fl_alert('Problem reading file where line is\n%s'%line)
                        
        
        elif choice.startswith('Print'):
                xdat = wavelengths[LOWDAT:DATLIMIT]
                ydat = intensities[LOWDAT:DATLIMIT]
                xrdplot = (('set xlabel "Wavelength (nm)"',
                          'set ylabel "Intensity"',
                          'set xrange [%f:%f]'%(ui.xaxis.minimum(), ui.xaxis.maximum()),
                          'set yrange [%f:%f]'%(ui.yaxis.minimum(), ui.yaxis.maximum()),
                          'set mxtics 5',
                          'set mytics 5',
                          'set grid xtics mxtics ytics mytics',
                          'set title "%s"'%usertime()),
                          (xdat, ydat, 'with lines notitle'))
                if choice.endswith('preview'):
                        device = '@'+VIEWER
                else:
                        device = SPOOLER
                
                if choice.startswith('Print x2'):
                        multiplot.doplots(device, xrdplot, xrdplot)
                else:
                        multiplot.doplots(device, xrdplot)
        
        elif choice == 'Calibration':
                fl_message('Cal is as good as it gets (1 nm)')
        
        elif choice == 'About':
                fl_message('Thorspec: A custom controller for the\n'
                           'Thor Labs Fibre spectrometers\n\n'
                           'version %s\nMark Colclough 2010-11'% VERSION)
        
        elif choice == 'Quit':
                sys.exit(0)


#======================== Choice Menu functions and data ========================
# Set the selected item on a (Choice) Fl_Menu_, and call the appropriate callback.
# item[2] is the callback, item[3] the udata.
def force_menu_choice(Menu, value):
        for i, item in enumerate(Menu.menu()):
                if value <= item[3]:
                        Menu.value(i)
                        item[2](Menu, item[3])
                        return
def yrange_cb(w,d):
        if d == 0: return 
        ui.yaxis.maximum(d/1000.0)
        ui.yaxis.minimum(0)

yrange_choices = (("0.001", 0, yrange_cb, 1, 0),
                   ("0.002", 0, yrange_cb, 2, 0),
                   ("0.005", 0, yrange_cb, 5, 0),
                   ("0.01", 0, yrange_cb, 10, 0),
                   ("0.02", 0, yrange_cb, 20, 0),
                   ("0.05", 0, yrange_cb, 50, 0),
                   ("0.1", 0, yrange_cb, 100, 0),
                   ("0.2", 0, yrange_cb, 200, 0),
                   ("0.5", 0, yrange_cb, 500, 0),
                   ("1.0", 0, yrange_cb, 1000, 0)
                   )

# Ensure all window close events kill the entire app, but ignore the escape key
# (don't just let the window hide until running callbacks finish - they
# may be running forever, and with no window, how are you going to stop them?)
def mainwin_cb(*args):
        if Fl.event() == FL_SHORTCUT and Fl.event_key() == FL_Escape:
               # That's the escape key. Ignore it.
                pass
        else:
                sys.exit()

def auto_scale(w=None):
        global intensities
        ui.canvas.pushzoom(WLMIN, 0, WLMAX, max(max(intensities), 0.001))
        # TODO leaves y range selector inconsistent (?), prob. should update it
        #force_menu_choice(ui.yrange, 1000*max(intensities));  # not sure about this

def get_new_data():
        global intensities
        if not ui.runbut.value() : return
        inttime = ui.inttime.value() / 1000.0
        device.setIntTime(inttime)   ## IOError if not initialised
        device.startScan()
        # The data line is permanently linked to the intensity array, so having
        # read in new data, all we have to do is redraw.
        device.getScanDataArray(intensities)
        ui.canvas.redraw()
        Fl.repeat_timeout(UPDATETIME, get_new_data)

def runbut_cb(w):
        get_new_data()


# Find device or decide to live without it (to replay recorded data only)
DEV_BAD = 0
DEV_OK = 1
DEV_ABSENT = 2
devstatus = DEV_BAD
while devstatus == DEV_BAD:
        try:
                spx.init()
                devstatus = DEV_OK
                device = spx
                print 'found spx'
        except IOError, e:
                try:
                        ccs.init()
                        devstatus = DEV_OK
                        device = ccs
                        print 'found ccs'
                except IOError, e:
                        # ignore = 0, retry = 1, quit = 2, close window = 0
                        ret = fl_choice('Failed to access spectrometer', 'Ignore', 'Retry', 'Quit')
                        if ret == 2:
                                sys.exit()
                        elif ret == 0:
                                devstatus = DEV_ABSENT
                                device = None
                                print 'no device'
                        # else retry

# General GUI setup
ui.make_window()
ui.yrange.align(FL_ALIGN_RIGHT)
ui.mainwin.size_range(640, 420) # min. size
ui.mainwin.resizable(ui.canvas)
ui.mainwin.callback(mainwin_cb)  # trap escape key, really exit
Fl_background(210, 210, 210)
ui.menubar.box(FL_THIN_UP_BOX)
gridmed = fl_gray_ramp(150 * (FL_NUM_GRAY - 1) / 255)
gridlight = fl_gray_ramp(190 * (FL_NUM_GRAY - 1) / 255)
ui.canvas.border(7)
ui.xaxis.major_grid_color(gridmed)
ui.xaxis.minor_grid_color(gridlight)
ui.xaxis.grid_visible(CA_MAJOR_GRID|CA_MINOR_GRID)
ui.yaxis.major_grid_color(gridmed)
ui.yaxis.minor_grid_color(gridlight)
ui.yaxis.grid_visible(CA_MAJOR_GRID|CA_MINOR_GRID)

if devstatus == DEV_ABSENT:
        ui.runbut.label("No device")
        ui.runbut.value(0)
        ui.runbut.deactivate()
        ui.yrange.deactivate()
        ui.inttime.deactivate()
else:
        ui.runbut.value(1)
        ui.runbut.callback(runbut_cb)

# Menu handling
ui.menubar.callback(menu_cb)

# Y range choice box
ui.yrange.menu(yrange_choices)
force_menu_choice(ui.yrange, 1000) # make graph and menu consistent on 1.000 range

# Auto scale button
ui.autobut.callback(auto_scale)

# Integration time control
ui.inttime.value(50)
ui.inttime.step(5)

# device dependent stuff
if device == spx:
        ui.inttime.bounds(5, 200)
        wavelengths = numpy.zeros(3000) # X data buffer for Ca_Line
        intensities = numpy.zeros(3000) # Y data buffer for Ca_Line
        LOWDAT = 107  # [LOWDAT:DATLIMIT] is the 400-800 nm slice of returned data
        DATLIMIT = 2712 # data outside this range may be wrong, so we ignore it
        # set axes in case not done by choice callback (min=max causes hang)
        WLMIN = 400
        WLMAX = 800
        ui.canvas.initzoom(WLMIN, 0, WLMAX, 1.0)
elif device == ccs:
        # TODO need to keep GUI alive if we allow such long times ui.inttime.bounds(5, 60000)
        ui.inttime.bounds(5, 200)
        wavelengths = numpy.zeros(3648)
        intensities = numpy.zeros(3648)
        LOWDAT = 0
        DATLIMIT = 3647
        WLMIN = 350
        WLMAX = 800
        ui.canvas.initzoom(WLMIN, 0, WLMAX, 1.0)
else:  # no device, so make data arrays to suit the biggest device (for recorded data only)
        wavelengths = numpy.zeros(3648)
        intensities = numpy.zeros(3648)
        LOWDAT = 0
        DATLIMIT = 3647
        WLMIN = 350
        WLMAX = 800
        ui.canvas.initzoom(WLMIN, 0, WLMAX, 1.0)


# Get the manufacturer's wavelength calibration
if device is not None:
        device.getWavelengthDataArray(wavelengths)

# Link a line on the plot to the wavelengths & intensities arrays
dataline = Ca_Line(wavelengths[LOWDAT:DATLIMIT], intensities[LOWDAT:DATLIMIT],
           FL_SOLID, 1, FL_BLUE, CA_NO_POINT)

# here we go
Fl.add_timeout(0.1, get_new_data)
ui.mainwin.show()
Fl.run()

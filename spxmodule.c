/* Python module to access Thorlabs SP1-USB and SP2-USB spectrometers */

#include <Python.h>
#include "spxdrv.h"

unsigned long inst = 0;                       // instrument handle
char errmsg[SPX_ERR_DESCR_BUFFER_SIZE];

#define CHECKINST  if (!inst) { \
		             PyErr_SetString(PyExc_IOError, "Instrument not initialised");\
		             return NULL;\
	               }

#define CHECKRET     if (ret != VI_SUCCESS){\
    	                 SPX_errorMessage(inst, ret, errmsg);\
    	                 PyErr_SetString(PyExc_IOError, errmsg);\
    	                 return NULL;\
                     }       

static PyObject *
spx_init(PyObject *self, PyObject *args) {
	long timeout = 3000;  // default 3000 ms timeout
	int ret;
	
    if (!PyArg_ParseTuple(args, "|l", &timeout)) return NULL;       
    ret = SPX_init("1313:0111", timeout, &inst);
    CHECKRET
    Py_RETURN_NONE;
}

static PyObject *
spx_close(PyObject *self, PyObject *args) {
	int ret;
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
    CHECKINST
	ret = SPX_close(inst);
    CHECKRET
    inst = 0;  // we don't have it any more
    Py_RETURN_NONE;
}

static PyObject *
spx_identificationQuery(PyObject *self, PyObject *args) {
	int ret;
	char vendor[SPX_BUFFER_SIZE];
	char name[SPX_BUFFER_SIZE];
	char serial[SPX_BUFFER_SIZE];
	char revision[SPX_BUFFER_SIZE];
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
    CHECKINST
	ret = SPX_identificationQuery(inst, vendor, name, serial, revision);
    CHECKRET
    return Py_BuildValue("ssss", vendor, name, serial, revision);
}
 
static PyObject *
spx_setIntTime(PyObject *self, PyObject *args) {
	int ret;
	double inttime;
	
	if (!PyArg_ParseTuple(args, "d", &inttime)) return NULL;
	CHECKINST
	ret = SPX_setIntTime (inst, inttime);
	CHECKRET
	return Py_BuildValue("");
}

static PyObject *
spx_getIntTime(PyObject *self, PyObject *args) {
	int ret;
	double inttime;
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
	ret = SPX_getIntTime(inst, &inttime);
	CHECKRET
	return Py_BuildValue("d", inttime);
}

static PyObject *
spx_startScan(PyObject *self, PyObject *args) {
	int ret;
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
	ret = SPX_startScan(inst);
	CHECKRET
	return Py_BuildValue("");
}


/* Return chars: I=idle; T=waiting for transfer; P=scan in progress (should never see)
 * W=waiting for ext trigger; U=unknown */
static PyObject *
spx_getDeviceStatus(PyObject *self, PyObject *args) {
	unsigned char stat;
	int ret;
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
    ret = SPX_getDeviceStatus(inst, &stat);
    CHECKRET
    if (stat == SPX_SCAN_STATE_IDLE) return Py_BuildValue("c", 'I');
    if (stat == SPX_SCAN_STATE_TRANSFER) return Py_BuildValue("c", 'T');
    if (stat == SPX_SCAN_STATE_IN_PROGRES) return Py_BuildValue("c", 'P');
    if (stat == SPX_SCAN_STATE_WAIT_TRIGGER) return Py_BuildValue("c", 'W');
    return Py_BuildValue("c", 'U');
}

/* Helper to create a list of doubles -- not needed if we use the array.array methods */
static PyObject *Convert_Double_Array(double array[], int length) {
    PyObject *pylist, *item;
    int i;
    
    pylist = PyList_New(length);
    if (pylist != NULL) {
        for (i=0; i<length; i++) {
            item = PyFloat_FromDouble(array[i]);
            PyList_SET_ITEM(pylist, i, item);
        }
    }
    return pylist;
}

static PyObject *
spx_getWavelengthData(PyObject *self, PyObject *args) {
	int ret;
	double wave[3000];
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
	ret = SPX_getWavelengthData(inst, wave, 0, 0);
	CHECKRET
	return Convert_Double_Array(wave, 3000);
}

// Read in wavelength data to a pre-existing array.array
static PyObject *
spx_getWavelengthDataArray(PyObject *self, PyObject *args) {
	char *bufp;
	int	buflen;
	int ret;
	
	if (!PyArg_ParseTuple(args, "w#", &bufp, &buflen)) return NULL;
	if (buflen != 3000*sizeof(double)) {
        PyErr_SetString(PyExc_TypeError, "Must provide array.array['d', range(3000)]");
		return NULL;
    }
	CHECKINST
	ret = SPX_getWavelengthData(inst, (double*)bufp, 0, 0);
	CHECKRET
	return Py_BuildValue("");
}

static PyObject *
spx_getScanData(PyObject *self, PyObject *args) {
	int ret;
	double data[3000];
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
	ret = SPX_getScanData(inst, data);
	CHECKRET
	return Convert_Double_Array(data, 3000);
}


// Read in data to a pre-existing array.array
static PyObject *
spx_getScanDataArray(PyObject *self, PyObject *args) {
	char *bufp;
	int	buflen;
	int ret;
	
	if (!PyArg_ParseTuple(args, "w#", &bufp, &buflen)) return NULL;
	if (buflen != 3000*sizeof(double)) {
        PyErr_SetString(PyExc_TypeError, "Must provide array.array['d', range(3000)]");
		return NULL;
    }
    CHECKINST
    ret = SPX_getScanData(inst, (double*)bufp);
	CHECKRET
	return Py_BuildValue("");
}

static PyMethodDef spxMethods[] = {
    {"init", spx_init, METH_VARARGS, "Initialise SPX"},
    {"close", spx_close, METH_VARARGS, "Close SPX"},
    {"identificationQuery", spx_identificationQuery, METH_VARARGS, "Get ID strings"},
    {"setIntTime", spx_setIntTime, METH_VARARGS, "Set integration time (s)"},
    {"getIntTime", spx_getIntTime, METH_VARARGS, "Get integration time (s)"},
    {"startScan", spx_startScan, METH_VARARGS, "Start single scan"},
    {"getDeviceStatus", spx_getDeviceStatus, METH_VARARGS, "Status:I(dle), T(ransfer awaited), P(rogress), W(ait trigger), U(nknown)"},
    {"getWavelengthData", spx_getWavelengthData, METH_VARARGS, "Get factory calibration"},
    {"getWavelengthDataArray", spx_getWavelengthDataArray, METH_VARARGS, "Get factory calibration into array.array"},
    {"getScanData", spx_getScanData, METH_VARARGS, "Get data"},
    {"getScanDataArray", spx_getScanDataArray, METH_VARARGS, "Get data into array.array"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initspx(void)
{
    (void) Py_InitModule("spx", spxMethods);
}

/* Python module to access Thorlabs CCS USB spectrometers */

#include <Python.h>
#include "CCS_Series_Drv.h"

unsigned long inst = 0;                       // instrument handle
char errmsg[CCS_SERIES_ERR_DESCR_BUFFER_SIZE];

#define CHECKINST  if (!inst) { \
		             PyErr_SetString(PyExc_IOError, "Instrument not initialised");\
		             return NULL;\
	               }

#define CHECKRET     if (ret != VI_SUCCESS){\
    	                 CCSseries_errorMessage(inst, ret, errmsg);\
    	                 PyErr_SetString(PyExc_IOError, errmsg);\
    	                 return NULL;\
                     }       

static PyObject *
ccs_init(PyObject *self, PyObject *args) {
    int ret;
    ret = CCSseries_init("1313:8081", 0, 0, &inst); //0,0 = don't idquery, resetdevice during init
    CHECKRET
    Py_RETURN_NONE;
}

static PyObject *
ccs_close(PyObject *self, PyObject *args) {
	int ret;
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
    CHECKINST
	ret = CCSseries_close(inst);
    CHECKRET
    inst = 0;  // we don't have it any more
    Py_RETURN_NONE;
}

static PyObject *
ccs_identificationQuery(PyObject *self, PyObject *args) {
	int ret;
	char vendor[CCS_SERIES_BUFFER_SIZE];
	char device[CCS_SERIES_BUFFER_SIZE];
	char serial[CCS_SERIES_BUFFER_SIZE];
	char revision[CCS_SERIES_BUFFER_SIZE];
        char driver_revision[CCS_SERIES_BUFFER_SIZE];
        
	if (!PyArg_ParseTuple(args, "")) return NULL;
    CHECKINST
	ret = CCSseries_identificationQuery(inst, vendor, device, serial, revision, driver_revision);
    CHECKRET
    return Py_BuildValue("ssss", vendor, device, serial, revision);
}
 
static PyObject *
ccs_setIntTime(PyObject *self, PyObject *args) {
	int ret;
	double inttime;
	
	if (!PyArg_ParseTuple(args, "d", &inttime)) return NULL;
	CHECKINST
        ret = CCSseries_setIntegrationTime(inst, inttime);
	CHECKRET
	return Py_BuildValue("");
}

static PyObject *
ccs_getIntTime(PyObject *self, PyObject *args) {
	int ret;
	double inttime;
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
        ret = CCSseries_getIntegrationTime(inst, &inttime);
	CHECKRET
	return Py_BuildValue("d", inttime);
}

static PyObject *
ccs_startScan(PyObject *self, PyObject *args) {
	int ret;
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
        ret = CCSseries_startScan(inst);
	CHECKRET
	return Py_BuildValue("");
}


/* Return chars: I=idle; T=waiting for transfer; G=triggered, S=starting
 * W=waiting for ext trigger; U=unknown */
static PyObject *
ccs_getDeviceStatus(PyObject *self, PyObject *args) {
	long stat;
	int ret;
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
    ret = CCSseries_getDeviceStatus(inst, &stat);
    CHECKRET
    if (stat == CCS_SERIES_STATUS_SCAN_IDLE) return Py_BuildValue("c", 'I');
    if (stat == CCS_SERIES_STATUS_SCAN_TRANSFER) return Py_BuildValue("c", 'T');
    if (stat == CCS_SERIES_STATUS_SCAN_TRIGGERED) return Py_BuildValue("c", 'G');
    if (stat == CCS_SERIES_STATUS_SCAN_START_TRANS) return Py_BuildValue("c", 'S');
    if (stat == CCS_SERIES_STATUS_WAIT_FOR_EXT_TRIG) return Py_BuildValue("c", 'W');
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
ccs_getWavelengthData(PyObject *self, PyObject *args) {
	int ret;
	double wave[CCS_SERIES_NUM_PIXELS];  // (3648)
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
        ret = CCSseries_getWavelengthData(inst, CCS_SERIES_CAL_DATA_SET_FACTORY, wave, NULL, NULL);
	CHECKRET
	return Convert_Double_Array(wave, CCS_SERIES_NUM_PIXELS);
}

// Read in wavelength data to a pre-existing array.array
static PyObject *
ccs_getWavelengthDataArray(PyObject *self, PyObject *args) {
	char *bufp;
	int	buflen;
	int ret;
	
	if (!PyArg_ParseTuple(args, "w#", &bufp, &buflen)) return NULL;
	if (buflen != CCS_SERIES_NUM_PIXELS*sizeof(double)) {
        PyErr_SetString(PyExc_TypeError, "Must provide array.array['d', range(3648)]");
		return NULL;
    }
	CHECKINST
        ret = CCSseries_getWavelengthData(inst, CCS_SERIES_CAL_DATA_SET_FACTORY, (double*)bufp, NULL, NULL);
	CHECKRET
	return Py_BuildValue("");
}

static PyObject *
ccs_getScanData(PyObject *self, PyObject *args) {
	int ret;
	double data[CCS_SERIES_NUM_PIXELS];
	
	if (!PyArg_ParseTuple(args, "")) return NULL;
	CHECKINST
        ret = CCSseries_getScanData(inst, data);
	CHECKRET
	return Convert_Double_Array(data, CCS_SERIES_NUM_PIXELS);
}


// Read in data to a pre-existing array.array
static PyObject *
ccs_getScanDataArray(PyObject *self, PyObject *args) {
	char *bufp;
	int	buflen;
	int ret;
	
	if (!PyArg_ParseTuple(args, "w#", &bufp, &buflen)) return NULL;
	if (buflen != CCS_SERIES_NUM_PIXELS*sizeof(double)) {
        PyErr_SetString(PyExc_TypeError, "Must provide array.array['d', range(3648)]");
		return NULL;
    }
    CHECKINST
    ret = CCSseries_getScanData(inst, (double*)bufp);
	CHECKRET
	return Py_BuildValue("");
}

static PyMethodDef ccsMethods[] = {
    {"init", ccs_init, METH_VARARGS, "Initialise CCS"},
    {"close", ccs_close, METH_VARARGS, "Close CCS"},
    {"identificationQuery", ccs_identificationQuery, METH_VARARGS, "Get ID strings"},
    {"setIntTime", ccs_setIntTime, METH_VARARGS, "Set integration time (s)"},
    {"getIntTime", ccs_getIntTime, METH_VARARGS, "Get integration time (s)"},
    {"startScan", ccs_startScan, METH_VARARGS, "Start single scan"},
    {"getDeviceStatus", ccs_getDeviceStatus, METH_VARARGS, "Status:I(dle), T(ransfer awaited), triGgered, S(tarting), W(ait trigger), U(nknown)"},
    {"getWavelengthData", ccs_getWavelengthData, METH_VARARGS, "Get factory calibration"},
    {"getWavelengthDataArray", ccs_getWavelengthDataArray, METH_VARARGS, "Get factory calibration into array.array"},
    {"getScanData", ccs_getScanData, METH_VARARGS, "Get data"},
    {"getScanDataArray", ccs_getScanDataArray, METH_VARARGS, "Get data into array.array"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initccs(void)
{
    (void) Py_InitModule("ccs", ccsMethods);
}

import os,sys
DNNC_ROOT=os.path.abspath(os.path.dirname(__file__))
sys.path.append(DNNC_ROOT)
sys.path.append(DNNC_ROOT+os.path.sep+'swig')
sys.path.append(DNNC_ROOT+os.path.sep+'python')

#from swig import dnnc
#from python import read_onnx

call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
cd TESTES

cl main_simd.cpp
cl original.cpp

cls
echo "Rodando o codigo original..."
original.exe
echo "Rodando o codigo modificado..."
main_simd.exe
echo "Rodando o codigo em OpenCL em CPU..."
cd MandelbrotCPU
MandelbrotCPU.exe
cd..
echo "Rodando o codigo em OpenCL em GPU..."
cd MandelbrotGPU
MandelbrotGPU.exe

pause

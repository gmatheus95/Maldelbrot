call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
cd TESTES

cl main_simd.cpp
cl original.cpp

cls
echo "Rodando o codigo original..."
original.exe
echo "Rodando o codigo modificado..."
main_simd.exe
echo "Rodando o codigo em OpenCL em CPU..."
openCL_CPU.exe
echo "Rodando o codigo em OpenCL em GPU..."
openCL_GPU.exe


pause

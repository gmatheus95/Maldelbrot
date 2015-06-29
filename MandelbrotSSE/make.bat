call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"

cl main_simd.cpp
cl original.cpp

cls
echo "Rodando o codigo original..."
original.exe
echo "Rodando o codigo modificado..."
main_simd.exe


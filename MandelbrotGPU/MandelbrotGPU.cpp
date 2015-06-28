/**********************************************************************
Copyright ©2014 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

// For clarity,error checking has been omitted.

#include <CL/cl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>

#define SUCCESS 0
#define FAILURE 1
#define IHEIGHT 8192
#define IWIDTH 8192

using namespace std;

/* convert the kernel file into a string */
int convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if(f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size+1];
		if(!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return 0;
	}
	cout<<"Error: failed to open file\n:"<<filename<<endl;
	return FAILURE;
}

int main(int argc, char* argv[])
{

	/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms;	//the NO. of platforms
	cl_platform_id platform = NULL;	//the chosen platform
	cl_int	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		cout << "Error: Getting platforms!" << endl;
		return FAILURE;
	}

	/*For clarity, choose the first available platform. */
	if(numPlatforms > 0)
	{
		cl_platform_id* platforms = (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);
		platform = platforms[0];
		free(platforms);
	}

	/*Step 2:Query the platform and choose the first GPU device if has one.Otherwise use the CPU as device.*/
	cl_uint				numDevices = 0;
	cl_device_id        *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);	
	if (numDevices == 0)	//no GPU available.
	{
		cout << "No GPU device available." << endl;
		cout << "Choose CPU as default device." << endl;
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);	
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);
	}
	else
	{
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	}
	

	/*Step 3: Create context.*/
	cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,NULL);
	
	/*Step 4: Creating command queue associate with the context.*/
	cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);

	/*Step 5: Create program object */
	const char *filename = "MandelbrotGPU_Kernel.cl";
	string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);
	
	/*Step 6: Build program. */
	status=clBuildProgram(program, 1,devices,NULL,NULL,NULL);

	//VERIFICA ERROS NO ARQUIVO CL (NECESSARIO!!!)
	if (status == CL_BUILD_PROGRAM_FAILURE) 
	{
		// Determine the size of the log
		size_t log_size;
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		// Allocate memory for the log
		char *log = (char *) malloc(log_size);

		// Get the log
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		// Print the log
		printf("%s\n", log);	
	}


	//inicializa variaveis para o calculo do Mandelbrot, cria arquivo, etc
	/* screen ( integer) coordinate */
    int iX,iY;
    int iXmax = IHEIGHT; 
    int iYmax = IWIDTH;
    /* world ( float) coordinate = parameter plane*/
    float CxMin=-2.5;
    float CxMax=1.5;
    float CyMin=-2.0;
    float CyMax=2.0;
    /* */
    float PixelWidth=(CxMax-CxMin)/iXmax;
    float PixelHeight=(CyMax-CyMin)/iYmax;
    /* color component ( R or G or B) is coded from 0 to 255 */
    /* it is 24 bit color RGB file */
    const int MaxColorComponentValue=255; 
    FILE * fp;
    char *imagename="mandelbrot.ppm";
    static unsigned char color[3];
    /*  */
    int IterationMax=256;
    /* bail-out value , radius of circle ;  */
    const float EscapeRadius=2;
    float ER2=EscapeRadius*EscapeRadius;
    /*create new file,give it a name and open it in binary mode  */
    fp= fopen(imagename,"wb"); /* b -  binary mode */
    /*write ASCII header to the file*/
    fprintf(fp,"P6\n %d\n %d\n %d\n",iXmax,iYmax,MaxColorComponentValue);
	
	//DEFINIR DADOS DE ENTRADA E SAIDA DA ITERACAO A SER PARARELIZADA!
	/*Step 7: Initial input,output for the host and create memory objects for the kernel*/
	size_t arraysize = 3*iXmax*iYmax; //tamanho do array gerado pós contas
	char *output = (char*) malloc(arraysize*sizeof(char));

	void *er2 = &ER2; //tem que ter um ponteiro de void apontando pro valor a ser passado para o inputBuffer
	void *cymin = &CyMin;
	void *ixmax = &iXmax;
	void *iterationmax = &IterationMax;
	void *cxmin = &CxMin;
	void *pixelwidth = &PixelWidth;
	void *pixelheigth = &PixelHeight;
	//buffers que farao contato entre memoria RAM (bloco principal) e VRAM (kernel)
	cl_mem input_CYMIN_Buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float),cymin, NULL);
	cl_mem input_CXMIN_Buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float),cxmin, NULL);
	cl_mem input_PIXELWIDTH_Buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float),pixelwidth, NULL);
	cl_mem input_PIXELHEIGTH_Buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float),pixelheigth, NULL);
	cl_mem input_IXMAX_Buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(int),(void *) ixmax, NULL);
	cl_mem input_ITERATIONMAX_Buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(int),(void *) iterationmax, NULL);
	cl_mem input_ER2_Buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(float),er2, NULL);
		
	//buffer de output ESTOU COM PROBLEMA NESSE BUFFER
	cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY , (arraysize + 1) * sizeof(char), NULL, NULL);

	/*Step 8: Create kernel object */
	cl_kernel kernel = clCreateKernel(program,"MandelbrotGPU", NULL);

	/*Step 9: Sets Kernel arguments.*/
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&input_CYMIN_Buffer);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&input_CXMIN_Buffer);
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&input_PIXELWIDTH_Buffer);
	status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&input_PIXELHEIGTH_Buffer);
	status = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&input_IXMAX_Buffer);
	status = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&input_ITERATIONMAX_Buffer);
	status = clSetKernelArg(kernel,6,sizeof(cl_mem), (void*)&input_ER2_Buffer);
	//ESTOU COM PROBLEMA NESSE BUFFER \/
	status = clSetKernelArg(kernel, 7, sizeof(cl_mem), (void *)&outputBuffer);
	
	/*Step 10: Running the kernel.*/
	size_t global_work_size[1] = {arraysize};
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);

	//ESTOU COM PROBLEMA NESSE BUFFER \/
	/*Step 11: Read the cout put back to host memory.*/
	status = clEnqueueReadBuffer(commandQueue, outputBuffer, CL_TRUE, 0, arraysize * sizeof(char), output, 0, NULL, NULL);
	
	output[arraysize] = '\0';	//Add the terminal character to the end of output. NAO SEI SE PRECISA!
	
	fwrite(output,1,arraysize,fp);


	/*Step 12: Clean the resources.*/
	status = clReleaseKernel(kernel);				//Release kernel.
	status = clReleaseProgram(program);				//Release the program object.
	status = clReleaseMemObject(input_CYMIN_Buffer);		//Release mem object.
	status = clReleaseMemObject(input_CXMIN_Buffer);
	status = clReleaseMemObject(input_PIXELWIDTH_Buffer);
	status = clReleaseMemObject(input_PIXELHEIGTH_Buffer);
	status = clReleaseMemObject(input_IXMAX_Buffer);
	status = clReleaseMemObject(input_ITERATIONMAX_Buffer);
	status = clReleaseMemObject(input_ER2_Buffer);
	status = clReleaseMemObject(outputBuffer);
	status = clReleaseCommandQueue(commandQueue);	//Release  Command queue.
	status = clReleaseContext(context);				//Release context.

	if (output != NULL)
	{
		//free(output);
		output = NULL;
	}

	if (devices != NULL)
	{
		free(devices);
		devices = NULL;
	}


	system("PAUSE");

	std::cout<<"Passed!\n";

	
	return SUCCESS;
}
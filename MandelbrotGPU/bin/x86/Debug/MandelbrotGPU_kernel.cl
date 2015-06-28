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
/*__kernel void MandelbrotGPU(__global float* in, __global float* out)
{
	int num = get_global_id(0);
	out[num] = in[num] + 1;
}*/

__kernel void MandelbrotGPU(__global float* cymin,__global float* cxmin,__global float* pixelwidth,__global float* pixelheigth,__global int* ixmax, __global int* iterationmax, __global float* er2, __global char* saida)
{
	int iY = get_global_id(0);
	int colorCounter = iY*(*ixmax)*3; //em qual parte do output vai ser salvo efetivamente as informações de um dos processamentos em paralelo
	float Cy = (*cymin) + iY*(*pixelheigth);
	float Cx;
	float Zx;
	float Zy;
	float Zx2;
	float Zy2;
	int Iteration;
    if (fabs(Cy)< (*pixelheigth)/2) 
		Cy = 0.0; /* Main antenna */
    for(int iX=0;iX<(*ixmax);iX++)
    {         
        Cx = (*cxmin) + iX*(*pixelwidth);
        /* initial value of orbit = critical point Z= 0 */
        Zx=0.0;
        Zy=0.0;
        Zx2=Zx*Zx;
        Zy2=Zy*Zy;
        /* */
        for (Iteration=0;Iteration<(*iterationmax) && ((Zx2+Zy2)<(*er2));Iteration++)
        {
            Zy=2*Zx*Zy + Cy;
            Zx=Zx2-Zy2 +Cx;
            Zx2=Zx*Zx;
            Zy2=Zy*Zy;
        };
        /* compute  pixel color (24 bit = 3 bytes) */
        if (Iteration==(*iterationmax))
        { /*  interior of Mandelbrot set = black */
            saida[colorCounter]=0;
            saida[colorCounter+1]=0;
            saida[colorCounter+2]=0;                           
        }
        else 
        { /* exterior of Mandelbrot set = white */
                saida[colorCounter]=(((*iterationmax)-Iteration) % 8) *  63;  /* Red */
                saida[colorCounter+1]=(((*iterationmax)-Iteration) % 4) * 127;  /* Green */ 
                saida[colorCounter+2]=(((*iterationmax)-Iteration) % 2) * 255;  /* Blue */
        };
        /*write color to the file*/
		        saida[colorCounter]=15;  /* Red */
                saida[colorCounter+1]=15;  /* Green */ 
                saida[colorCounter+2]=15;  /* Blue */

		colorCounter+= 3;
	}


}




#include <time.h>
#include <stdio.h>
#include <math.h>
#include <xmmintrin.h>
#include <iostream>

int main()
{
	/* Variaveis do tipo Clock para medir o tempo de execucao*/
	clock_t start, end;

	/* Limites de x e y*/
	const int iXmax = 8000;
	const int iYmax = 8000;
	
	/* Definindo vetores Cxf e CyF (float) para usar nos registradores XMMX
	   o nome das variaveis foi alterado em comparacao a versao original disponibilizada
	   de Cx -> CxF , Cy -> CyF , pois como discutido Cx tem o mesmo nome de um registrador
	   algo que pode resultar em problemas ao utilizar inline. */
	float CxF[4], CyF[4];
	
	/* Limites de CxF e CyF */
	const float CxMin = -2.5;
	const float CxMax = 1.5;
	const float CyMin = -2.0;
	const float CyMax = 2.0;
	
	/* Definindo dimensoes do pixel */
	float PixelWidth = (CxMax - CxMin) / iXmax;
	float PixelHeight = (CyMax - CyMin) / iYmax;

	/* As componentes de cores RGB variam de 0 a 255*/
	const int MaxColorComponentValue = 255;


	FILE * fp;
	char *filename = "mandelbrot_simd_sse.ppm";

	float Zx, Zy;
	float Zx2, Zy2;
	/*  */
	int Iteration;
	const int IterationMax = 256;
	/* Valor de saida , raio do circulo */
	const float EscapeRadius = 2;
	float ER2 = EscapeRadius*EscapeRadius;
	/*Criando,nomeando e abrindo o arquivo em modo binario*/
	fp = fopen(filename, "wb");
	/*Escreve o ASCII header no arquivo */
	fprintf(fp, "P6\n %d\n %d\n %d\n", iXmax, iYmax, MaxColorComponentValue);
	/* Vetor em que sera armazenado os valores dos pixels da imagem */
	static unsigned char color[3];

	/* Definindo vetores que serao usados para operacoes SIMD*/
	__m128 incrementador = _mm_set1_ps(4.0f);
	__m128 PW = _mm_set1_ps(PixelWidth);
	__m128 minCx = _mm_set1_ps(CxMin);
	__m128 PH = _mm_set1_ps(PixelHeight);
	__m128 minCy = _mm_set1_ps(CyMin);
	__m128 iY = _mm_set_ps(0.0f, 1.0f, 2.0f, 3.0f);
	__m128 iX = _mm_set_ps(0.0f, 1.0f, 2.0f, 3.0f);
	/* Indice usado para posicionar os valores no array color*/

	/* Comeca a contar o tempo de execucao */
	start = clock();
	for (int k = 0; k < iYmax / 4; k++)
	{
		/* Calculando CyF utilizando SIMD , dessa forma calculamos 4 valores de CyF por vez */
		__asm {
				movaps xmm0, incrementador
				movaps xmm1, PH
				movaps xmm2, minCy
				movaps xmm3, iY
				mulps xmm3, xmm1
				addps xmm3, xmm2
				movups[CyF], xmm3
				movaps xmm3, iY
				addps xmm3, xmm0
				movups[iY], xmm3
		}
		for (int w = 0; w < 4; w++)
		{
			iX = _mm_set_ps(0.0f, 1.0f, 2.0f, 3.0f);
			if (fabs(CyF[w]) < PixelHeight / 2) CyF[w] = 0.0;
			for (int i = 0; i < iXmax / 4; i++)
			{
				/* Calculando CxF utilizando SIMD , dessa forma calculamos 4 valores de CxF por vez */
				__asm {
						movaps xmm0, incrementador
						movaps xmm1, PW
						movaps xmm2, minCx
						movaps xmm3, iX
						mulps xmm3, xmm1
						addps xmm3, xmm2
						movups[CxF], xmm3
						movaps xmm3, iX
						addps xmm3, xmm0
						movups[iX], xmm3
				}
				for (int j = 0; j < 4; j++)
				{
					Zx = 0.0;
					Zy = 0.0;
					Zx2 = 0.0;
					Zy2 = 0.0;
					for (Iteration = 0; Iteration < IterationMax && ((Zx2 + Zy2) < ER2); Iteration++)
					{
						Zy = 2 * Zx*Zy + CyF[w];
						Zx = Zx2 - Zy2 + CxF[j];
						Zx2 = Zx*Zx;
						Zy2 = Zy*Zy;
					};
					if (Iteration == IterationMax)
					{ /*  interior do Mandelbrot = black */
						color[0] = 0;
						color[1] = 0;
						color[2] = 0;
					}
					else
					{ /* exterior do Mandelbrot = white */

						color[0] = ((IterationMax - Iteration) % 8) * 63;  /* Red */
						color[1] = ((IterationMax - Iteration) % 4) * 127;  /* Green */
						color[2] = ((IterationMax - Iteration) % 2) * 255;  /* Blue */
					};
					fwrite(color, 1, 3, fp);				
				}
			}
		}
	}
	/* Fechando arquivo e calculando o tempo de execucao*/
	end = clock();
	fclose(fp);
	float cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
	printf("TEMPO SSE = %f segundos\n", cpu_time_used);
	return 0;
}
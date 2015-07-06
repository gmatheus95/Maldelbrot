# Projeto Final de Arquitetura e Organização de Computadores 2

O projeto teve como objetivo desenvolver versões paralelas do algoritmo de Mandelbrot e analisar seus desempenhos em diferentes computadores. As formas de paralelização escolhidas para desenvolvimento foram: 
* Instruções SIMD SSE
* AMD OpenCL (para GPU AMD Radeon, com CPU Branch, caso não haja tal GPU)

# Metodologia

* De acordo com o código fornecido pelo professor (dentro da pasta MandelbrotOriginal), desenvolvemos algoritmos que mantivessem a mesma lógica dos cálculos e, por consequência, gerassem o mesmo resultado. O resultado é uma imagem de fractal gerada em arquivo .ppm de 8000x8000 pixels de resolução, mostrado na figura abaixo:

      ![alt tag](https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/Saida.PNG)

* Pela limitação dos registradores na arquitetura SSE, as versões paralelizadas utilizaram variáveis do tipo float, ao invés de double, como no código base.
* Foram realizados 10 testes em 4 computadores, com as seguintes configurações:
  * <b>1</b>: AMD Phenon II X4 840 3,2Ghz (4 Cores), 4Gb RAM DDR3 1066Mhz, AMD Radeon HD 6750
  * <b>2</b>: Intel Core2 Duo T5870 2Ghz (2 Cores), 2Gb RAM DDR2 800Mhz, Intel HD Graphics
  * <b>3</b>: Intel Core2 Duo E7400 3,06Ghz (2 Cores), 4Gb RAM DDR2 1066Mhz, AMD Radeon HD 7750
  * <b>4</b>: AMD FX 6100 Black Edition 3,3Ghz (6 Cores), 6Gb RAM DDR3 1333Mhz, AMD Radeon HD 7850

# Paralelização SIMD SSE

Um dos métodos de paralelização usado no nosso trabalho foi o de instruções SIMD SSE, o primeiro problema encontrado
foi que as instruções SSE usam variáveis do tipo float, enquanto o código original usa variáveis do tipo double, portanto
o tipo das variáveis usados nos códigos foi alterada para suportar as instruções SSE.
A seguir precisavamos definir os loops que seriam paralelizados. 

O programa possui 3 loops principais:

<b>loop 1:</b>

```C++
for(iY=0;iY<iYmax;iY++)
 {
    Cy=CyMin + iY*PixelHeight;
    if (fabs(Cy)< PixelHeight/2) Cy=0.0; / Main antenna /
    //...
```

<b>loop 2:</b>
```C++
  for(iX=0;iX<iXmax;iX++)
  {
    Cx=CxMin + iX*PixelWidth;
    //...
```

<b>loop 3:</b>
```C++
  for (Iteration=0;Iteration<IterationMax && ((Zx2+Zy2)<ER2);Iteration++)
  {
      Zy=2*Zx*Zy + Cy;
      Zx=Zx2-Zy2 +Cx;
      Zx2=Zx*Zx;
      Zy2=Zy*Zy;
  };
```

Dos 3 loops apresentados, decidimos paralelizar os loop 1 e 2 , o loop 3 não pode ser paralelizado pois ele depende
de iterações anteriores. Os loops paralelizados (loops 1 e 2) ficaram com o seguinte formato:

<b>loop 1:</b>
```C++
for (int k = 0; k < iYmax / 4; k++)
 {
  / Calculando CyF utilizando SIMD , dessa forma calculamos 4 valores de CyF por vez /
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
```

<b>loop 2:</b>
```C++
for (int i = 0; i < iXmax / 4; i++)
  {
   / Calculando CxF utilizando SIMD , dessa forma calculamos 4 valores de CxF por vez /
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
```
# Paralelização OpenCL

Como não tinhamos nenhuma experiência com OpenCL, foi necessário um estudo da linguagem e das implementações existentes. Assim, encontramos o AMD APP SDK, que é uma coleção de bibliotecas e exemplos de implementação desenvolvida em OpenCL, todos os códigos devidamente documentados. Os códigos implementados foram projetados para executar em GPUs AMD Radeon com suporta a OpenCL, e continham também o CPU Branch, ou seja, caso tal GPU não estivesse disponível, utulizaria-se a CPU para execução do algoritmo. 

Com isso em mãos, o código desenvolvido em OpenCL para o Mandelbrot foi adaptado do código fornecido pela AMD do programa “Hello World” em OpenCL (Todos os direitos de redistribuição de código estão explicitados no código-fonte MandelbrotGPU.cpp e MandelbrotGPU_kernel.cl).

O OpenCL utiliza-se, em linhas gerais, de uma estrutura de kernel, que seria como uma função, cujos parâmetros são buffers de entrada e saída, e que executa em paralelo em um número determinado de instâncias, onde a diferença entre uma instância e outra é um índice.
Assim, observando as iterações do código original:
```C++
for(iY=0;iY<iYmax;iY++)
{
     Cy=CyMin + iY*PixelHeight;
     if (fabs(Cy)< PixelHeight/2) Cy=0.0; /* Main antenna */
     for(iX=0;iX<iXmax;iX++)
     {
                Cx=CxMin + iX*PixelWidth;
                /* initial value of orbit = critical point Z= 0 */
                Zx=0.0;
                Zy=0.0;
                Zx2=Zx*Zx;
                Zy2=Zy*Zy;
                /* */
                for (Iteration=0;Iteration<IterationMax && ((Zx2+Zy2)<ER2);Iteration++)
                {
                    Zy=2*Zx*Zy + Cy;
                    Zx=Zx2-Zy2 +Cx;
                    Zx2=Zx*Zx;
                    Zy2=Zy*Zy;
                };
                /* compute  pixel color (24 bit = 3 bytes) */
                if (Iteration==IterationMax)
                { /*  interior of Mandelbrot set = black */
                   color[0]=0;
                   color[1]=0;
                   color[2]=0;
                }
                else
                { /* exterior of Mandelbrot set = white */
                     color[0]=((IterationMax-Iteration) % 8) *  63;  /* Red */
                     color[1]=((IterationMax-Iteration) % 4) * 127;  /* Green */
                     color[2]=((IterationMax-Iteration) % 2) * 255;  /* Blue */
                };
                /*write color to the file*/
                fwrite(color,1,3,fp);
        }
}
```
optou-se por paralelizar o laço de repetição controlado pela variável iY, de forma que tal loop fosse executado em “iY” kernels simultaneamente gerando resultados em buffers de saída do tipo “*char” em posições referentes ao índice do kernel (que na prática seria o índice da variável “i” do código original). O código do kernel se encontra abaixo:

```C++
__kernel void MandelbrotGPU(__global float* cymin,__global float* cxmin,__global float* pixelwidth,__global float* pixelheigth,__global int* ixmax, __global int* iterationmax, __global float* er2,__global char* saida, __global char* saida2)
{
	bool flag = false;
	int iY = get_global_id(0);
	int colorCounter = iY*(*ixmax)*3;
	int limear = (*ixmax)/2;
	 //em qual parte do output vai ser salvo efetivamente as informações de um dos processamentos em paralelo
	if (iY >= (limear)) //considerando ixmax == iymax
	{
		flag = true;
		colorCounter = (iY-(limear))*(*ixmax)*3;
	}
	//calculo do mandelbrot
	float Cy = (*cymin) + iY*(*pixelheigth);
	float Cx = 0;
	float Zx = 0;
	float Zy= 0;
	float Zx2 = 0;
	float Zy2 = 0;
	int Iteration = 0;
	char color[3];
    if (fabs(Cy)< (*pixelheigth)/2) 
		Cy = 0.0; /* Main antenna */
    for(int iX=0;iX<(*ixmax);iX++)
    {         
        Cx = (*cxmin) + iX*(*pixelwidth);
        /* initial value of orbit = critical point Z= 0 */
        Zx=0.0;
        Zy=0.0;
        Zx2=0;
        Zy2=0;
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
				color[0]=0;
				color[1]=0;
				color[2]=0;                           
        }
        else 
		{ /* exterior of Mandelbrot set = white */			
			color[0]=(((*iterationmax)-Iteration) % 8) *  63;  /* Red */
            color[1]=(((*iterationmax)-Iteration) % 4) * 127;  /* Green */ 
            color[2]=(((*iterationmax)-Iteration) % 2) * 255;  /* Blue */
        };

		//a flag ve se a saida vai pro primeiro ou pro segundo buffer dependendo do iY sendo processado por esse kernel
		if (!flag)
		{
			saida[colorCounter] = color[0];//color[0];
			saida[colorCounter+1] = color[1];//color[1];
			saida[colorCounter+2] = color[2];//color[2];
		}
		else
		{
			saida2[colorCounter] = color[0];
			saida2[colorCounter+1] = color[1];
			saida2[colorCounter+2] = color[2];
		}
		colorCounter+= 3;
	}
}
```

Dessa forma, a ideia básica da implementação em OpenCL do algoritmo de Mandelbrot é a seguinte:

* Procura quantos e quais CPUs e GPUs estão efetivamente disponíveis para execução do programa
* Determina que, se houver placa gráfica, ela será utilizada para a execução, senão, será utilizada a CPU
* Cria e constrói o arquivo .cl que será o kernel do projeto
* Atribui valor inicial às variáveis utilizadas no cálculo do Mandelbrot e cria o arquivo “.ppm”
* Cria os buffers de entrada (valores constantes utilizados em toda iteração em paralelo) e buffers de saída (dois buffers, cada um contendo metade do arquivo .ppm ao fim da execução, ou seja, guardam os resultados dos cálculos paralelos).
* Executa o Kernel em “iY” instâncias (em paralelo) recebendo os buffers de entrada constantes e modificando os buffers de saída (de acordo com o índice do kernel) com o resultado dos cálculos de cada kernel
* Armazena em variáveis locais os conteúdos dos buffers de saída (foram utilizados dois para garantir que o limite de tamanho do buffer intrinseco ao hardware utilizado não fosse excedido)
* Escreve no arquivo .ppm os resultados
* Desaloca todos os buffers e as instâncias de kernel da memória
* Fim de execução 

<b>O código se apresenta devidamente comentado, explicitando tais etapas de maneira mais detalhada.</b>

<b> Observação: </b> Para compilar o projeto OpenCL, são necessários o AMD APP SDK versão 2.9.1 (<link> http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/ </link>) e Microsoft Visual Studio 2010 ou superior. Caso haja, mesmo assim, erros na compilação, estão disponíveis versões <i> Release</i> na pasta TESTES deste repositório.

#Resultados

Para comparar desempenho entre as versões, 10 testes foram realizados em cada computador como citado anteriormente e
os resultados obtidos foram os seguintes:

![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC1Tabela.PNG) 
![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC2Tabela.PNG)
![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC3Tabela.PNG) 
![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC4Tabela.PNG)

<b>A partir dessas tabelas os seguintes gráficos foram montados:</b>

![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC1Cpu.PNG)
![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC1Gpu.PNG)

![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC2Cpu.PNG)

![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC3Cpu.PNG)
![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC3Gpu.PNG)

![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC4Cpu.PNG)
![] (https://github.com/gmatheus95/Maldelbrot/blob/master/Imagens/PC4Gpu.PNG)

#Conclusão

Como é possivel observar através dos gráficos e tabelas, em todos os computadores testados, tanto a versão SSE quanto a versão OpenCL foram mais eficientes que a original. 

Fatos interessantes:
* Já era esperado que o OpenCL, quando executado em GPU, apresentaria <i>performance</i> superior aos algoritmos executados em CPU, uma vez que as GPUs têm centenas de núcleos de processamento.
* Quando executado em CPU, o OpenCL obteve melhor desempenho por utilizar-se de todos os núcleos de processamento, diferentemente das versões original e SIMD, que apresentaram uso de apenas um dos núcleos (aferido por gerenciador de tarefas do Windows).

Dessa forma, é possivel concluir que todas as paralelizações aumentaram a eficiência do codigo original, como desejado.






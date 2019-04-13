# Image_Denoising_Using_Mpi
I implemented a parallel algorithm for denoising the noised image within the Ising model using Metropolis-Hastings algorithm.

# Procedure
First, you should install "openmpi" and have "gcc compiler" in your machine.

Secondly, you should arrange the path according to

	- export PATH=$PATH:$HOME/openmpi/bin 
	- export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/openmpi/lib

# Execution
Then, to run this program, an example command is given

  - mpicc -g imagedenoise.c -o imagedenoise
  - mpiexec -n 11 ./imagedenoise input.txt output.txt 0.4 0.15
  
Hint: (mpiexec -n [NUM_OF_PROCESSOR + 1] ./imagedenoise input output beta pi)
   
# Notes
NOTE1: The program can execute with any given processor number. 
	(But, I recommend you to choose 10 (actually 10+1=11) processors 
  due to the trade-off between message passing and iteration count) 
	Eventually, you can choose any number of processors that you wish.
	
NOTE2:	I added two printf statements into code to show that the code is 
  continuing the calculation.
	Example console output:
  
		- mpicc -g imagedenoise.c -o imagedenoise
		- mpiexec -n 11 ./imagedenoise input.txt output.txt 0.4 0.15
		-  ---tasks are sended to slaves---
		-  // after about 30 sec or 50 sec (in virtual machine).
		-  ---the noisy image is denoised---
		-  // denoised image is created in the same directory with the given output name
		-  // and the program is ended

NOTE3: There is an example input file named "lena200_noisy.txt" which is represented in the Ising Model. 
And also there are python scripts in order to convert image to txt or vice versa.

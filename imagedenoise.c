/*
Notes			: To run this this program you should give command like in example;
				: mpicc -g imagedenoise.c -o imagedenoise
				: mpiexec -n 11 ./imagedenoise noisy.txt denoised.txt 0.4 0.15
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
#include <stdbool.h>
// it finds the min of given two values
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
// constants for calculating the probabilitiy acceptance
double beta;
double pi;
double gama;
// constant for (acceptance probability) limit that will be used to flip or not flip 
double aprob;
//input and output txt
char* input;
char* output;
int size;

int main(int argc, char** argv){
	input=argv[1];
	output=argv[2];
	beta=atof(argv[3]);
	pi=atof(argv[4]);
	gama=(0.5)*log((1-pi)/pi);
	
	// size x size matris
	size=200;

	// mpi initialize  
	MPI_Init(NULL, NULL);
  	int world_rank;
  	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
  	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	
	// N is the number of slave processes	
	int N = world_size-1;
  	int i, j;
  	
	// split the master and slave processes
	MPI_Comm slaves;
	MPI_Comm_split( MPI_COMM_WORLD, ( world_rank == 0 ), world_rank, &slaves );
	
	// master works
  	if (world_rank==0){
    	// arr is an array for getting noisy image integers    	
		int** arr = NULL;
    	arr = (int **)malloc(sizeof(int*)*size);
    	for(i = 0 ; i < size ; i++)
     		arr[i] = (int *)malloc(sizeof(int)*size);
   
		// input file reading
		FILE *file;
		file=fopen(input,"r");
		for(i=0;i<size;i++)
			for(j=0;j<size;j++) 
        		fscanf(file,"%d",&arr[i][j]);
		
		fclose(file);	

		// send tasks to slave processes
		printf("\n ---tasks are sended to slaves--- \n");
		for(i = 1 ; i <= N ; i++)
     		for(j = 0 ; j < (size/N) ; j++)
        		MPI_Send(arr[(size/N)*(i-1)+j], size, MPI_INT, i, j, MPI_COMM_WORLD);

		// receice the output from slave processes	
		for(i = 1 ; i <= N ; i++)
      		for(j = 0 ; j < (size/N) ; j++)
        		MPI_Recv(arr[(size/N)*(i-1)+j], size, MPI_INT, i, j, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		// write the output to the output.txt file
		file=fopen(output,"w");
		for(i=0;i<size;i++){
			for(j=0;j<size;j++) 
        		fprintf(file,"%d ",arr[i][j]);
			fprintf(file,"\n");
		}
		fclose(file);
  		
		//at the end
		printf("\n ---the noisy image is denoised--- \n");
		free(arr);
	}
	// slave works
  	else{
		// subarr is an array for getting task from master to each slaves
    	int** subarr = NULL;
    	subarr = (int **)malloc(sizeof(int*)*(size/N));
		for(i = 0 ; i < (size/N) ; i++)
      		subarr[i] = (int *)malloc(sizeof(int)*size);
		
		// receive tasks from master  process
    	for(i = 0 ; i < (size/N) ; i++)
      		MPI_Recv(subarr[i], size, MPI_INT, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    	
		// slaves share their top and bottom parts of data to use when we choose a border pixel
		for(i = 1 ; i <= N ; i++){
			if(i!=world_rank){
      			MPI_Send(subarr[0], size, MPI_INT, i, 0+(size/N), MPI_COMM_WORLD);
				MPI_Send(subarr[(size/N)-1], size, MPI_INT, i, ((size/N)-1)+(size/N), MPI_COMM_WORLD);
			}	
		}
	
		// wait for all slaves to send their data		
		MPI_Barrier(slaves);
	
		// a copy array for calculation
		int** noise = NULL;
    	noise = (int **)malloc(sizeof(int*)*(size/N));
		for(i = 0 ; i < (size/N) ; i++)
      		noise[i] = (int *)malloc(sizeof(int)*size);	

		for(i = 0 ; i < (size/N) ; i++)
      		for(j = 0 ; j < size ; j++)
        		noise[i][j]=subarr[i][j];
	
		// a temporary array for shared data between slaves
		int** temp = NULL;
		// 2 means-> 0: for coming data from upper process(lower rank)
		// 2 means-> 1: for coming data from lower process(upper rank)
    	temp = (int **)malloc(sizeof(int)*2);
		for(i = 0 ; i < 2 ; i++)
      		temp[i] = (int *)malloc(sizeof(int)*size);

		// booleans for deciding to send messages when there is any changes in the top or bottom row-pixel of each processes' data
		bool change=false;
		bool change1=false;
		bool change2=false;
	
		// for random generation 
		srand ( time ( NULL));
		
		// starts iteration whether flip the pixel or not
		for(int ite=0;ite<(500000/N);ite++){
			// filling temp if any border row is changed or if it is first coming, it gets the data sended before out of the iteration
			if(change || ite==0 ){
				// if it is first process gets the data from next higher rank
				if(world_rank==1)
					MPI_Recv(temp[1], size, MPI_INT, (world_rank+1), (0+(size/N)), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					
				// if it is last process gets the data from next lower rank
				else if(world_rank==N)
					MPI_Recv(temp[0], size, MPI_INT, (world_rank-1), ((size/N)-1)+(size/N), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				
				// if it is in between gets the data from next lower and higher rank
				else{
					MPI_Recv(temp[0], size, MPI_INT, (world_rank-1), ((size/N)-1)+(size/N), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					MPI_Recv(temp[1], size, MPI_INT, (world_rank+1), (0+(size/N)), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				}
				// change is setted false again
				change=false;
				// wait for all slaves to get the changed data
				MPI_Barrier(slaves);
			}
			
			// giving random index numbers for choosing a pixel
			int i = rand() % (size/N);
			int j = rand() % size;
			
			// summation for around of pixel  
			int sum;
			
			// pixel is in first row	
			if(i==0){
				// if it is first process
				if(world_rank==1){
					// if it is first column					
					if(j==0)
						sum=noise[i+1][j]+noise[i][j+1]+noise[i+1][j+1];
					// if it is last column					
					else if(j==size-1)
						sum=noise[i+1][j]+noise[i][j-1]+noise[i+1][j-1];
					// if it is in between					
					else
						sum=noise[i+1][j]+noise[i][j+1]+noise[i+1][j+1]+noise[i][j-1]+noise[i+1][j-1];
					
				}
				// if it is other process then get the data from neighbor process's border
				else{
					if(j==0)
						sum=noise[i+1][j]+noise[i][j+1]+noise[i+1][j+1]+temp[0][j]+temp[0][j+1];
					else if(j==size-1)
						sum=noise[i+1][j]+noise[i][j-1]+noise[i+1][j-1]+temp[0][j]+temp[0][j-1];
					else
						sum=noise[i+1][j]+noise[i][j+1]+noise[i+1][j+1]+noise[i][j-1]+noise[i+1][j-1]+temp[0][j]+temp[0][j+1]+temp[0][j-1];
				
				}
				// a border pixel is chosen
				change1=true;
			}
			// pixel is in last row	
			else if(i==((size/N)-1)){
				// if it is last process
				if(world_rank==N){				
					// if it is first column					
					if(j==0)
						sum=noise[i-1][j]+noise[i][j+1]+noise[i-1][j+1];
					// if it is last column					
					else if(j==size-1)
						sum=noise[i-1][j]+noise[i][j-1]+noise[i-1][j-1];
					// if it is in between					
					else
						sum=noise[i-1][j]+noise[i][j+1]+noise[i-1][j+1]+noise[i][j-1]+noise[i-1][j-1];
				}
				// if it is other process then get the data from neighbor process's border
				else{
					if(j==0)
						sum=noise[i-1][j]+noise[i][j+1]+noise[i-1][j+1]+temp[1][j]+temp[1][j+1];
					else if(j==size-1)
						sum=noise[i-1][j]+noise[i][j-1]+noise[i-1][j-1]+temp[1][j]+temp[1][j-1];
					else
						sum=noise[i-1][j]+noise[i][j+1]+noise[i-1][j+1]+noise[i][j-1]+noise[i-1][j-1]+temp[1][j]+temp[1][j+1]+temp[1][j-1];
				
				}
				// a border process is chosen
				change2=true;
			}
			// pixel is in between	
			else{
				if(j==0)
					sum=noise[i+1][j]+noise[i][j+1]+noise[i+1][j+1]+noise[i-1][j]+noise[i-1][j+1];
				else if(j==size-1)
					sum=noise[i+1][j]+noise[i][j-1]+noise[i+1][j-1]+noise[i-1][j]+noise[i-1][j-1];
				else
					sum=noise[i+1][j]+noise[i][j+1]+noise[i+1][j+1]+noise[i-1][j]+noise[i-1][j+1]+noise[i][j-1]+noise[i+1][j-1]+noise[i-1][j-1];
				
			}

			// calculation of the acceptance probability
			double acceptance;
			acceptance=exp((-2.0)*gama*noise[i][j]*subarr[i][j]+(-2.0)*beta*noise[i][j]*sum);
			
			// probability cannot excess 1			
			acceptance=MIN(1.0,acceptance);
			
			// random acceptance probability limit between 0 and 1
			aprob = (double)rand()/RAND_MAX;

			// decide whether flip the pixel or not	according to aprob
			if(acceptance>aprob)
				noise[i][j]=-noise[i][j];
	
			// if it is not the last iteration 
			if(ite!=((500000/N)-1)){
				// if any border pixel is chosen
				if(change1 || change2){
					// send the newest data to the others
					for(int i = 1 ; i <= N ; i++){
						if(i!=world_rank){
				  			MPI_Send(noise[0], size, MPI_INT, i, 0+(size/N), MPI_COMM_WORLD);
							MPI_Send(noise[(size/N)-1], size, MPI_INT, i, ((size/N)-1)+(size/N), MPI_COMM_WORLD);
						}	
					}
	
					// wait for all slaves to send their message		
					MPI_Barrier(slaves);
					// update changes
					change1=false;
					change2=false;
					// it is for the next iteration to decide is there any message or not
					change=true;
				}
			}
		}
		
		// output is ready. take the values from copy array to real array
		for(int k = 0 ; k < (size/N) ; k++)
		  for(int l = 0 ; l < size ; l++)
		    subarr[k][l]=noise[k][l];
		
		// send the output back to the master process
		for(j = 0 ; j < (size/N) ; j++)
		    MPI_Send(subarr[j], size, MPI_INT, 0, j, MPI_COMM_WORLD);

		// wait for all slave processes send their data
		MPI_Barrier(slaves);
		free(subarr);
		free(temp);
	}

	// wait for all processes are done
	MPI_Barrier(MPI_COMM_WORLD);
	// terminate all processes
	MPI_Finalize();
}


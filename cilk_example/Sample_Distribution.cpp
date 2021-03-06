#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
//#include <cilk/cilk.h>
//#include <cilk/cilk_api.h>
//#include "get_time.h"
#include <cmath>
using namespace std;
#define THRESHOLD_OF_TRANSPOSE 100
#define THRESHOLD_OF_DISTRIBUTION 100
int Pick[100001],Sample[100001],Offset[100001],InsertPointer[100001];
int log2_up(int k){
    int a = 0;
    while (k){
        k = k >> 1;
        a++;
    }
    return a-1;
}
inline uint32_t hash32(uint32_t a) {
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	if (a<0) a = -a;
	return a;
}
bool Verification(int* A, int n){
    for (int i = 0; i<n-1;i++)
        if (A[i]>A[i+1])    return false;
    return true;
}
void Merge(int* A, int n, int* C, int m){
    int i=0,j=0;
    while( i<n || j<m){
        while ( j<m && A[i]>Sample[j]) {j++;}//cout<<i<<" "<<j<<"\n";};
        if (j==m) j--;
        //cout<<"\n";
        while ( i<n && A[i]<=Sample[j]) {i++;C[j]++;}//cout<<i<<" "<<j<<"\n";}
        if (i==n)   return;
    }
}
int reduce(int* A, int n) {
	if (n < 2) {
      int ret = 0;
      for (int i = 0; i < n; i++) ret += A[i];
      return ret;
    }
	int L, R;
	/*L = cilk_spawn reduce(A, n/2);
	R = reduce(A+n/2, n-n/2);
	cilk_sync;*/
    L = reduce(A, n/2); R = reduce(A+n/2, n-n/2);
	return L+R;
}
void Move(int* A, int* B, int bucket_size, int* D, int buckets){
    for (int i = 0,p=0; i<buckets; i++){
        for (int j = 0; j<D[i];j++)
            B[InsertPointer[i]++] = A[p++];
    }
}
void Move(int* A, int* B, int buckets){
    
}
void Transpose(int* C, int n, int x, int lx, int y, int ly) {
    if ((lx <= THRESHOLD_OF_TRANSPOSE) && (ly <= THRESHOLD_OF_TRANSPOSE)) {
        for (int i = x; i<x+lx; i++)
            for (int j = y; j<y+ly; j++)
                if(i<j){
                    int tmp = C[(n*j) + i];
                    C[(n*j) + i] = C[(n*i) + j];
                    C[(n*i) + j] = tmp;
                    //cout <<"Transfer: "<<"x="<<x<<" y="<<y<<endl;
                }
    }
    else if (lx >= ly){
        int midx = lx/2;
        Transpose(C,n,x, midx, y, ly);
        Transpose(C,n,x+midx, lx-midx, y, ly);
    }else{
        int midy = ly/2;
        Transpose(C,n,x,lx,y,midy);
        Transpose(C,n,x,lx,y+midy,ly-midy);
    }
}
void Sample_Sort(int* A, int* B, int* C, int* D, int n){
    int bucket_quotient = 4;
    int buckets = sqrt(n);
    int bucket_size = n / buckets;

    //--------------------Step 1--------------------
    for (int i = 0; i<buckets; i++){
        if (i == buckets - 1)
            sort(A+(buckets-1)*bucket_size,A+n);
        else
            sort(A+i*bucket_size,A+(i+1)*bucket_size);
        
    }
    //for (int i = 0;i<n;i++) cout<< A[i]<<" "; cout<<endl;
    
    
    //---------------------Step 2--------------------
    int logn = log2_up(n);
    int random_pick = bucket_quotient * buckets * logn;
            //-------Randomly Pick cRootnLogn samples
    for (int i = 0;i<random_pick;i++)
        Pick[i] = A[hash32(i)%n];           //for (int i = 0;i<random_pick;i++) cout<<Pick[i]<<" "; cout<<"\n";
    sort(Pick, Pick+random_pick);
            //-------Randomly Pick every cLogn samples
    for (int i = 0, j=0; j<buckets-1;  j++,i+=bucket_quotient * logn)
        Sample[j] = Pick[i];
    Sample[buckets - 1] = INT_MAX;      //for (int i = 0; i < buckets; i++) cout<< Sample[i]<<" "; cout<<"\n";
    
    
    //-----------------Step 3--------------------
        //First, get the count for each subarray in each bucket. I store them in C
    for (int i = 0; i < buckets; i++){
        //cout<<"-----------Bucket "<<i/buckets<<"------------------\n"; for (int j = i;j<i+buckets;j++) cout<<A[j]<<" "; cout<<"\n";
        if (i == buckets-1) 
            Merge(A+i*bucket_size, n-i*bucket_size ,C+i*buckets,buckets);
        else
            Merge(A+i*bucket_size, bucket_size ,C+i*buckets,buckets);
        //for (int j = i;j<i+buckets;j++) cout<<C[j]<<" "; cout<<"\n";
    }
    for (int i = 0; i<buckets*buckets;i++) D[i] = C[i];
    
        //Then, transpose the array     
    //for (int i = 0; i < buckets*buckets; i++) if ((i+1)%buckets == 0) cout<< C[i]<<"\n"; else cout<< C[i]<<" "; cout<<"\n";
    Transpose(C,buckets,0,buckets,0,buckets);
    //for (int i = 0; i < buckets*buckets; i++) if ((i+1)%buckets == 0) cout<< C[i]<<"\n"; else cout<< C[i]<<" "; cout<<"\n";
    
        //scan to compute the offsets
    InsertPointer[0] = Offset[0] = 0;
    for (int i = 1; i<buckets; i++)
        InsertPointer[i] = Offset[i] = reduce(C+(i-1)*buckets,buckets)+Offset[i-1];
    //for (int i = 0; i<buckets; i++) cout<<InsertPointer[i]<<" "; cout<<"\n";
    
        //Lastly, move each element to the corresponding bucket
    for (int i = 0; i<buckets; i++){
        if (i == buckets-1)
            Move(A+i*bucket_size, B, n-i*bucket_size, D+i*buckets, buckets);
        else
            Move(A+i*bucket_size, B, bucket_size, D+i*buckets, buckets);
    }

    //-----------------Step 4--------------------
    for (int i = 0; i<buckets; i++){
        if (i == buckets-1)
            sort(B+Offset[i],B+n);
        else
            sort(B+Offset[i],B+Offset[i+1]);
    }
}

int main(int argc, char** argv) {
	if (argc < 2) {
		cout << "Usage: ./reduce [num_elements] [distribution=1]" << endl;
		return 0;
	}
	int n = atoi(argv[1]);
	int* A = new int[n];
    int* B = new int[n];
    int* C = new int[n];
    int* D = new int[n];
    for (int i = 0; i < n; i++) C[i] = 0;
	//cilk_for (int i = 0; i < n; i++) A[i] = i;
    for (int i = 0,j=n; i < n; i++,j--) A[i] = j;
    //Verification(A,n);
    cout << "Data Generation is Complete, Start Sorting and Timing!\n";
	//timer t; t.start();
    
    Sample_Sort(A,B,C,D,n);
	//t.stop();
	//cout << "sorting time: " << t.get_total() << "\nStarting Verification Now!\n";
	if (Verification(B,n))  cout<<"The result is correct!" << endl; else cout<<"The result is incorrect!" << endl;
	
    return 0;
}

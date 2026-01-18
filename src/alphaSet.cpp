#include <iostream>
#include <math.h>

int Calcsn(int n, long int s1, long int &nN, float& a)
{
	long int si,si0,sn,sSum;
	
	sSum = s1;
	si0 = 0;
	for(int i=2; i<=n; i++)
	{
		si = 0.5*(s1 + std::sqrt(pow(s1,2)+4*s1*sSum));
		sSum += si;
		a = float(si)/float(sSum);
		
	}
	
	sSum = s1;
	si0 = 0;
	for(int i=2; i<=n; i++)
	{
		si = 0.5*(s1 + sqrt(pow(s1,2)+4*s1*sSum));
		sSum += si;
		a = float(si)/float(sSum);
		
	}
	nN = si;
	return si;
}

int main(int argc, char* argv[]) {
	
	

	int loops = 10;
	long int initN = 80000;
	long int nNeutrons = initN;
	float alpha = 1.0;
	if(argc >= 2)
	{
		loops = std::stoi(argv[1]);
		initN = std::stoi(argv[2]);
	}
	else
	{
		std::cerr << "Invalid number of input arguments\n"
		<< "Two arguments are required: actualLoop initialNumberOfNeutrons \n";
		return -1;
	}
	Calcsn(loops,initN, nNeutrons, alpha);

	std::cout << alpha << "\n";
	std::cout << nNeutrons << "\n";
	return 0;
}
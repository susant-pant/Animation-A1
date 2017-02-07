
#include <iostream>
#include "Vec3f.h"
#include "Vec3f_FileIO.h"

int main()
{ 
	std::string file( "./test.txt" );

	VectorContainerVec3f vecs;
	loadVec3fFromFile( vecs, file );

	for( auto & v : vecs )
	{
		std::cout << v << std::endl;
	}

	std::string file2( "./noNime.con" );

	VectorContainerVec3f vecs2;
	loadVec3fFromFile( vecs2, file2 );

	for( auto & v : vecs2 )
	{
		std::cout << v << std::endl;
	}
}

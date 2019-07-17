#include <iostream>
#include <string>         
#include "itkImageFileReader.h"
#include "GetPot.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkImage.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataWriter.h"
#include "itkDefaultStaticMeshTraits.h"
#include "itkMesh.h"
#include "itkPolylineCell.h"
#include "TrkVTKPolyDataFilter.txx"
#include "PolylineMeshToVTKPolyDataFilter.h"
#include "itkImageDuplicator.h"
#include "itkNeighborhoodIterator.h"
#include <time.h>

#include <iostream>
#include "itkImage.h"
#include <map>
#include "itkDefaultStaticMeshTraits.h"
#include "itkMesh.h"
#include "itkTriangleCell.h"
#include <set>
#include "GetPot.h"
#include <string>
#include "colortab.h"
#include "fsenv.h"
 
#include "mrisurf.h"
#include "itkVTKPolyDataWriter.h"
#include "mris_multimodal_refinement.h"
#include "itkImage.h"
#include "vtkSmartPointer.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "itkImageFileReader.h"

#include <vtkPolyLine.h>
#include <vtkPolyData.h>

int main(int narg, char*  arg[])
{
//	try{

		constexpr unsigned int Dimension = 3;
		typedef double CoordType;
		typedef itk::Mesh< CoordType, Dimension > MeshType;
		typedef MeshType::PointsContainer PointsContainer;
		typedef MeshType::PointType PointType;
		//typedef MeshType::PointIdentifier PointIdentifier;
		typedef MeshType::CellType CellType;
		typedef itk::TriangleCell< CellType > TriangleType;

		GetPot cl(narg, const_cast<char**>(arg));
		if(cl.size()==1 || cl.search(2,"--help","-h"))
		{
			std::cout<<"Usage: " << std::endl;
			std::cout<< arg[0] << " -i surface -o surface -n -l gradientDescentlambda -k iterations -m numberOfImages image1 image2 image3"  << std::endl;   
			return -1;
		}
		const char *inSurf= cl.follow ("", "-i");
		const char *outSurf = cl.follow ("", "-o");
		const char *outNormals = cl.follow ("", "-n");
		float lambda= cl.follow (0.001, "-l");
		int iterations = cl.follow (10, "-k");


		MRI_SURFACE *surf;
		surf = MRISread(inSurf);

		int imageNumber = cl.follow(0,"-m");
		
		std::vector<MRI*> images; 

		MRIS_MultimodalRefinement* meh = new MRIS_MultimodalRefinement();

		std::vector<std::string> fileNames;
		for(;imageNumber>0;imageNumber--)
		{
			fileNames.push_back(cl.next(""));
			MRI *imageFS =  MRIread(fileNames[fileNames.size()-1].c_str()) ;
			images.push_back(imageFS);
			meh->addImage(imageFS);
		}

		meh->getTarget(surf);
		
		double x,y,z;
		vtkSmartPointer<vtkPoints> points = vtkPoints::New();
		int totalPoints=0;
  		vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
		typedef itk::Image<float,3> ImageType;
		typedef itk::ImageFileReader<itk::Image<float,3>> ImageReaderType;
		ImageReaderType::Pointer reader = ImageReaderType::New();
		reader->SetFileName ( fileNames[0]);
		reader->Update();
	
		ImageType::Pointer image = reader->GetOutput();

		for (unsigned j=0;j<surf->nvertices;j++)
		{
			
			double x,y,z, nx,ny,nz;

			MRISsurfaceRASToVoxel(surf, images[0], surf->vertices[j].x,surf->vertices[j].y,surf->vertices[j].z, &x,&y,&z);
			MRISsurfaceRASToVoxel(surf, images[0], surf->vertices[j].nx,surf->vertices[j].ny,surf->vertices[j].nz, &nx,&ny,&nz);
			//MRIsurfaceRASToVoxel(surf, surf->vertices[j].x,surf->vertices[j].y,surf->vertices[j].z, &x,&y,&z);
			//MRIsurfaceRASToVoxel(surf, surf->vertices[j].nx,surf->vertices[j].ny,surf->vertices[j].nz, &nx,&ny,&nz);

			ImageType::IndexType index;
			index[0]=x;
			index[1]=y;
			index[2]=z;
	
			ImageType::PointType point, normal;
			image->TransformIndexToPhysicalPoint(index, point);
			index[0]=nx;
			index[1]=ny;
			index[2]=nz;
	
			image->TransformIndexToPhysicalPoint(index, normal);
			vtkIdType *ids = new vtkIdType [2];
			points->InsertPoint (totalPoints,point[0],point[1],point[2]);
			ids[0] = totalPoints;
			totalPoints++;
			points->InsertPoint (totalPoints, point[0]+normal[0],point[1]+normal[1],point[2]+normal[2]);
			ids[1] = totalPoints;
			totalPoints++;
			
			vtkSmartPointer<vtkPolyLine> polyLine =    vtkSmartPointer<vtkPolyLine>::New();
			polyLine->GetPointIds()->SetNumberOfIds(2);
			polyLine->GetPointIds()->SetId(0,ids[0]);
			polyLine->GetPointIds()->SetId(1,ids[1]);
		//	vtk->InsertNextCell (VTK_POLY_LINE, 2, ids);
			cells->InsertNextCell(polyLine);
		}

                 vtkSmartPointer<vtkPolyData> polyData  = vtkSmartPointer<vtkPolyData>::New();
                 polyData->SetPoints(points);
                polyData->SetLines(cells);

			itk::SmartPointer<TrkVTKPolyDataFilter<ImageType>> vtk2trk  = TrkVTKPolyDataFilter<ImageType>::New();
		vtk2trk->SetReferenceImage(reader->GetOutput());		
		vtk2trk->SetInput( polyData);
		vtk2trk->VTKToTrk(outNormals);

		MRISwrite(surf,outSurf);		
		MRISfree(&surf);	


	/*
	}catch(...)
	{
		std::cout << "Error --> ";
		for(int i=0;i<narg;i++)
		{
			std::cout << arg[i];
		}
		std::cout << std::endl;

		return EXIT_FAILURE;
	}*/
	return EXIT_SUCCESS;
}
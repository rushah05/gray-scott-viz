#include <adios2.h>
#include <mpi.h>
#include <string>
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {

  MPI_Init(&argc, &argv);
  int comm_size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  std::string pb_filename = argv[1];
  //std::string xml_filename = argv[2];

  std::string output_bp_filename = argv[2];

  int compressU = std::atoi(argv[3]); // -1: have not stored U; 0: have stored U
  int compressV = std::atoi(argv[4]); // -1: have not stored V; 0: have stored V;
 
  int useSST = std::atoi(argv[5]);

  //std::cout << "compressU = " << compressU << std::endl;
  //std::cout << "compressV = " << compressV << std::endl;       

  adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
  //const std::string input_fname = "gs.bp";
  adios2::IO inIO = adios.DeclareIO("CompressedSimulationOutput");
  inIO.SetEngine("BP4");
  if (useSST == 1) {
      inIO.SetEngine("SST");
  }
  adios2::Engine reader = inIO.Open(pb_filename, adios2::Mode::Read);
  
  adios2::Variable<double> inVarU;
  adios2::Variable<double> inVarV;

  adios2::Dims shapeU;
  adios2::Dims shapeV;
  
  adios2::IO outIO = adios.DeclareIO("DecompressedSimulationOutput");
  outIO.SetEngine("BP4");
  adios2::Engine writer = outIO.Open(output_bp_filename, adios2::Mode::Write);

  adios2::Variable<double> varU;
  adios2::Variable<double> varV;

  reader.BeginStep();
  writer.BeginStep();
  
  adios2::Variable<int> inVarStep = inIO.InquireVariable<int>("step");
  adios2::Variable<int> step = outIO.DefineVariable<int>("step");

  if (compressU > -1) {
       //std::cout << "compressU" << std::endl;
       inVarU  = inIO.InquireVariable<double>("U");
       shapeU = inVarU.Shape();
  }
  if (compressV > -1) {
       //std::cout << "compressV" << std::endl;
       inVarV = inIO.InquireVariable<double>("V");
       shapeV = inVarV.Shape();
  }
  if (compressU > -1) {
       varU = outIO.DefineVariable<double>("U", {shapeU[0], shapeU[1], shapeU[2]}, {0,0,0}, {shapeU[0], shapeU[1], shapeU[2]});
  }
  if (compressV > -1) {
       varV = outIO.DefineVariable<double>("V", {shapeV[0], shapeV[1], shapeV[2]}, {0,0,0}, {shapeV[0], shapeV[1], shapeV[2]});                                                                                                                                                                         }

  
  double io_time = 0.0;
  int iter = 0;
  while (true) {
      if (iter > 0) {
          if (reader.BeginStep() != adios2::StepStatus::OK ||
              writer.BeginStep() != adios2::StepStatus::OK) {
              break;
          }
      }
      
      inVarStep = inIO.InquireVariable<int>("step");
     
      std::vector<int> inStep;
      reader.Get(inVarStep, inStep, adios2::Mode::Sync);
      writer.Put<int>(step, inStep[0], adios2::Mode::Sync);
      
      //std::cout << "step = " << inStep[0] << std::endl;
 
      if (compressU > -1) {
          //std::cout << "compressU" << std::endl;
          inVarU  = inIO.InquireVariable<double>("U");
          //shapeU = inVarU.Shape();
      }
      if (compressV > -1) {
          //std::cout << "compressV" << std::endl;
          inVarV = inIO.InquireVariable<double>("V");
          //shapeV = inVarV.Shape();
      }
   
      std::vector<double> u;
      std::vector<double> v;
      if (compressU > -1) {
	std::cout << inStep[0] << ",";
	auto start = std::chrono::high_resolution_clock::now();
        reader.Get(inVarU, u, adios2::Mode::Sync);
	auto stop = std::chrono::high_resolution_clock::now();
        io_time = std::chrono::duration<double>(stop-start).count();
        std::cout << "," << io_time << std::endl;
	writer.Put<double>(varU, u.data(), adios2::Mode::Sync);
      }
      if (compressV > -1) {
	std::cout << inStep[0] << ",";
        auto start = std::chrono::high_resolution_clock::now();
        reader.Get(inVarV, v, adios2::Mode::Sync);
        auto stop = std::chrono::high_resolution_clock::now();
        io_time = std::chrono::duration<double>(stop-start).count();
	std::cout << "," << io_time << std::endl;
	writer.Put<double>(varV, v.data(), adios2::Mode::Sync);
      
      }

      iter++;
      reader.EndStep();
      writer.EndStep();

  }
  //std::cout << io_time << std::endl;
  reader.Close();
  writer.Close();
  MPI_Finalize();
  return 0;
}

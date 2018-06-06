#if (defined _DEBUG) && (defined _MSC_VER) && (defined VLD)
#include "vld.h"
#endif /* Run Visual Leak Detector during Debug */

#ifndef USE_MONGODB
#define USE_MONGODB
#endif /* USE_MONGODB */

#include "ManagementProcess.h"
#include "CalculateProcess.h"
#include "parallel.h"

int main(int argc, const char** argv) {
    /// Parse input arguments
    InputArgs* input_args = InputArgs::Init(argc, argv);
    if (nullptr == input_args) { exit(EXIT_FAILURE); }
    /// Register GDAL
    GDALAllRegister();
    /// Initialize of MPI environment
    int size;
    int rank;
    MPI_Init(NULL, NULL);
    {
        MPI_Comm_size(MCW, &size);
        MPI_Comm_rank(MCW, &rank);

        try {
            if (rank == MASTER_RANK) {
                ManagementProcess(input_args);
            }
            CalculateProcess(rank, size, input_args);
        } catch (ModelException& e) {
            cout << e.what() << endl;
            MPI_Abort(MCW, 3);
        }
        catch (std::exception& e) {
            cout << e.what() << endl;
            MPI_Abort(MCW, 4);
        }
        catch (...) {
            cout << "Unknown exception occurred!" << endl;
            MPI_Abort(MCW, 5);
        }
    }
    /// clean up
    delete input_args;
    /// Finalize the MPI environment and exit with success
    MPI_Finalize();
    return 0;
}

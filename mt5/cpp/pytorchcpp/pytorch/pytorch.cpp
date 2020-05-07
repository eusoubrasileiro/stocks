#pragma warning (disable : 4146)
#include <iostream>
#include <torch\torch.h>
#include <torch/cuda.h>

#include "pytorchcpp.h"
#define PYTORCHCPP_DLL

#include <fstream> // debugging dll load by metatrader 5 output to txt file -> located where it started
std::ofstream debugfile("pytorchcpp.txt");

namespace th = torch;

auto dtype32_option = th::TensorOptions().dtype(th::kFloat32).requires_grad(false);
auto dtype64_option = th::TensorOptions().dtype(th::kFloat64).requires_grad(false);
auto dtypeL_option = th::TensorOptions().dtype(th::kLong).requires_grad(false);

th::Tensor thfdcoefs;
th::Device deviceCPU = th::Device(th::kCPU);
th::Device deviceifGPU = deviceCPU;

// calculate the fractdif coeficients to be used next
// $$ \omega_{k} = -\omega_{k-1}\frac{d-k+1}{k} $$
// output is allocated inside to a pytorch tensor
// on current device
void setFracDifCoefs(double d, int size){
    th::NoGradGuard guard; // same as with torch.no_grad(): block
    double *w = new double[size];
    w[0] = 1.;
    for(int k=1; k<size; k++)
        w[k]=-w[k-1]/k*(d-k+1);
    std::reverse(w, w+size);
    thfdcoefs = th::from_blob(w, {1, 1, size}, dtype64_option);
	// to GPU or not
    thfdcoefs.to(deviceifGPU);
}

// apply fracdif filter on signal array
// FracDifCoefs must be supplied
// output is allocated inside
int FracDifApply(double signal[], int size, double output[]){
  th::NoGradGuard guard; // same as with torch.no_grad(): block
  th::Tensor thdata = th::from_blob(signal, { 1, 1, size}, dtype64_option).clone();
  // to GPU or not
  thdata.to(deviceifGPU);
  th::Tensor thresult = th::conv1d(thdata, thfdcoefs).reshape({ -1 });
  // back to CPU
  thresult = thresult.to(deviceCPU);
  // double array output
  double* ptr_data = thresult.data_ptr<double>();
  int outsize = thresult.size(0);
  memcpy(output, ptr_data, sizeof(double) * outsize);

  return outsize;
}


#include <mutex>

std::mutex m; // to ensure Metatrader only calls on GPU

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            if (th::cuda::is_available()) {
              //std::cout << "CUDA is available! Running on GPU." << std::endl;
                deviceifGPU = th::Device(th::kCUDA);
            }
            //debugfile << "process attach" << std::endl;
            break;
        case DLL_PROCESS_DETACH:
            // detach from process
            //debugfile << "process deattach" << std::endl;
            break;
        case DLL_THREAD_ATTACH:
            // attach from thread
            //debugfile << "thread  attach" << std::endl;
            break;
        case DLL_THREAD_DETACH:
            // detach from thread
            //debugfile << "thread  deattach" << std::endl;
            break;
    }
    return TRUE; // succesful
}

#define GIGABytes 1073741824.0

auto dtype_option = dtype32_option;

// Batch Cholesky based on
// https://www.pugetsystems.com/labs/hpc/PyTorch-for-Scientific-Computing---Quantum-Mechanics-Example-Part-3-Code-Optimizations---Batched-Matrix-Operations-Cholesky-Decomposition-and-Inverse-1225/
inline th::Tensor Cholesky(th::Tensor &A){
    th::NoGradGuard guard; // same as with torch.no_grad(): block

    auto L = th::zeros_like(A, dtype_option.device(deviceifGPU));
    auto s = th::zeros(A.size(0), dtype_option.device(deviceifGPU));

    for (int i = 0; i < A.size(-1); i++) {
        for (int j = 0; j < i + 1; j++) {
            s.fill_(0);
            for (int k = 0; k < j; k++) {
                s.add_(L.select(1, i).select(-1, k) * L.select(1, j).select(-1, k));
            }
            if (i == j)
                L.select(1, i).select(-1, j).copy_(th::sqrt(A.select(1, i).select(-1, i) - s));
            else
                L.select(1, i).select(-1, j).copy_(1.0 / L.select(1, j).select(-1, j) * (A.select(1, i).select(-1, j) - s));
        }
    }
  //
  // for i in range(A.shape[-1]):
  //     for j in range(i+1):
  //         s = 0.0
  //         for k in range(j):
  //             s = s + L[...,i,k] * L[...,j,k]
  //
  //         L[...,i,j] = th.sqrt(A[...,i,i] - s) if (i == j) else \
  //                   (1.0 / L[...,j,j] * (A[...,i,j] - s))
  // return L
    return L;
}

#pragma once

#include <vector>
#include <tuple>
#include <torch/torch.h>

////Using Dataset to overcome low GPU usage: GPU spikes with big space (delay) in between
//
////Supremum Augmented dickey fuller test OLS matrices as a Dataset
////One sample is composed of all OLS's data needed to perform the ADF's to calculate one sadf(t) point
//class SADFtMatrices : public torch::data::Dataset<SADFtMatrices>
//{
//    private:
//        bool verbose;
//        int maxw;
//        int minw;
//        int p; // order of AR Model
//        th::Tensor X; // Mather X OLS matrix, all OLS adf matrices are inside this
//        th::Tensor z;
//        int adfs_count; // number of adfs for one sadf t
//        int nobsadf; //number of observations(maximum - first adf)
//        int nsadft; // number of sadf t's to calculate the entire SADF
//
//    public:
//        explicit SADFtMatrices(float *signal, int n, int maxw, int minw, int p = 30, bool verbose = false){
//            maxw = maxw;
//            minw = minw;
//            p = p;
//            verbose = verbose;
//            th::NoGradGuard guard; // same as with torch.no_grad(): block
//
//            auto nobs = n - p - 1; // data used for regression
//            X = th::zeros({ nobs, 3 + p }, dtype_option);
//            auto y = th::from_blob(signal, { n }, dtype_option);
//
//            auto diffilter = th::tensor({ -1, 1 }, dtype_option).view({ 1, 1, 2 }); // first difference filter
//            auto dy = th::conv1d(y.view({ 1, 1, -1 }), diffilter).view({ -1 });
//            z = dy.slice(0, p).clone();
//
//            // fill in first 3 columns, drit, trend, reg. data
//            // acessors or tensor[i][j].item<int>()
//            auto ay = y.accessor<float, 1>();
//            auto aX = X.accessor<float, 2>(); // drift
//            auto ady = dy.accessor<float, 1>();
//            for (auto i = 0; i < nobs; i++) {
//                aX[i][0] = 1; // drift term
//                aX[i][1] = p + 1 + i; // deterministic trend
//                aX[i][2] = ay[p + i];  // regression data(nobs)
//            }
//            // fill in other columns, start at third
//            for (auto j = 0; j < nobs; j++)
//                for (auto i = 1; i < p + 1; i++) //X[:, 2+i] = dy[p-i:-i]
//                    aX[j][2 + i] = ady[p - i + j];
//
//            adfs_count = maxw - minw;
//            auto nadf = maxw; //  data used is (maximum - first adf)
//            nobsadf = nadf - p - 1;
//            nsadft = n - maxw;
//            p = p;
//
//            if (verbose) {
//                std::cout << "ADF tests calculated for one sadf(t) " << adfs_count << std::endl;
//                std::cout << "Number of sadf(t) points " <<  nsadft << std::endl;
//            }
//
//        };
//
//        // Override the get method to load custom data.
//        // index :
//        // index of a specific sadf(t) point calculation
//        // same as start line for master main X OLS matrix / z vector
//        // returns all statistic tests elements of all ADF's to calculate this sadf(t) point
//        // tuple(OLS X, OLS z, nobservations per ADF)
//        torch::data::Example<> get(size_t index) override {
//            th::NoGradGuard guard; // same as with torch.no_grad(): block
//
//            auto nobts = th::zeros({ adfs_count }, dtype_option.device(deviceCPU));
//
//            // Xm // master X for this sadft - biggest adf OLS X matrix
//            // zm // master Z for this sadft(biggest adf OLS independent term)
//            auto Xm = X.narrow(0, index, nobsadf); //  master X for this sadft - biggest adf OLS X matrix
//            auto zm = z.narrow(0, index, nobsadf); // master Z for this sadft (biggest adf OLS independent term
//            auto Xbts = Xm.repeat({ adfs_count, 1 }).view({ adfs_count, nobsadf, (3 + p) });
//            auto zbts = zm.repeat({ adfs_count, 1 }).view({ adfs_count, nobsadf });
//
//            for (int k = 0; k < adfs_count; k++) { //each is smaller than previous
//                //    sadf loop until minw, every matrix is smaller than the previous
//                //    zeroing i lines, observations are becomming less also
//                Xbts.select(0, k).narrow(0, 0, k).fill_(0);
//                zbts.select(0, k).narrow(0, 0, k).fill_(0);
//                nobts.select(0, k).fill_(float(nobsadf - k - (p + 3)));
//            }
//
//            // due problem design with this class not accepting 3 objects as a return
//            // have to create fake target adding 1 item on zm vector
//            // tha corresponds to the number of observations of the ADF test
//            auto fake_target = th::zeros({ adfs_count, nobsadf + 1 });
//            fake_target.narrow(1, 0, nobsadf).copy_(zbts);
//            fake_target.narrow(1, nobsadf, 1).view(-1).copy_(nobts);
//
//            // is not needed according to
//            // https://discuss.pytorch.org/t/how-to-manually-delete-free-a-tensor-in-aten/64153/4?u=eusouoandre
//            //delete []&nobts;
//            //delete []&zbts;
//
//            return {Xbts, fake_target};
//        };
//
//        // Override the size method to infer the size of the data set.
//        torch::optional<size_t> size() const override {
//            return nsadft;
//        };
//};


// supremum augmented dickey fuller test SADF
// expands backward many adfs for each point
// using minw window size and maxw as maximum backward size
// lag - which ADF backward lag gave the highest ADF
int sadf(float* signal, float* outsadf, float* outadfmaxidx, int n, int maxw, int minw, int order, bool drift, float gpumem_gb, bool verbose) {
    // working perfectly - only one GPU so only one thread can access it a time
    std::lock_guard<std::mutex> lock(m); 

    if (th::cuda::is_available()) {
        //std::cout << "CUDA is available! Running on GPU." << std::endl;
        deviceifGPU = th::Device(th::kCUDA);
    }

#ifdef  DEBUG
    try {
#endif //  DEBUG
        th::NoGradGuard guard; // same as with torch.no_grad(): block

      // fastest version
      //     - assembly rows of OLS problem using entire input data
      //     - send batchs of 1GB adfs tests to GPU until entire
      //     sadf is calculated, last batch might (should be smaller)

        // number of params of the AR model
        // 1. drift term (DC) - default
        // 2. deterministic trend (optional)
        // 3. gama or coeficient of y(t-1) - mandatory
        // 4. aditional order of AR model beyond t-2 and included
        int params = order + 2 + 1 * drift;
        // check for minimum number of data for the t statistical test
        // degrees of freedom cannot be zero -> neq_adf - params
        // auto neq_adf_min = minw - order - 1; //number of equations for shortest adf (minw)
        // neq_adf_min - params > 0 or minw - order - 1 - params > 0 -> minw > order + 1 + params
        if (minw <= order + 1 + params) {
            if (verbose) std::cout << "error minw must be > order + 1 + params: " << order + 1 + params << std::endl;
            return 0;
        }

        if (minw > maxw) {
            if (verbose) std::cout << "error minw > maxw " << std::endl;
            return 0;
        }

        auto neq = n - order - 1; // number of equations (lines)
        //auto nobs = n - order - 1; // data used for regression
        auto X = th::zeros({ neq, params }, dtype_option);
        auto y = th::from_blob(signal, { n }, dtype_option);
        // dont know why but conv1d is causing a SEH exception
        // replaced it by a vector subtraction
        //auto diffilter = th::tensor([-1, 1 }, dtype_option).view({ 1, 1, 2 }); // first difference filter
        auto dy = y.narrow(0, 1, n-1) - y.narrow(0, 0, n-1);
        //auto dy = th::conv1d(y.view({ 1, 1, -1 }), th::zeros(10)).view({ -1 });
        auto z = dy.slice(0, order).clone();

        // fill in first 3 columns, drit, trend, reg. data
        X.select(1, 0).fill_(1); // first column 1 - drift term
        if (drift) {
            X.select(1, 1).copy_(th::arange(order + 1, n)); // deterministic trend
            X.select(1, 2).copy_(y.narrow(0, order, neq)); // regression data(nobs)
        }
        else { // without drift term
            X.select(1, 1).copy_(y.narrow(0, order, neq)); // regression data(nobs)
        }
        // fill in other params columns, starting from the last column
        // ignoring the first param columns already filled
        for (auto i = params - 1, j = 0; i >= 2 + 1 * drift; i--, j++) //X[:, 2+i] = dy[p-i:-i]
            X.select(1, i).copy_(dy.narrow(0, j, neq));
        //   i = params - 1
        //    for j in range(params - 3) :
        //       print(i, j)
        //       X.select(1, i).copy_(dy.narrow(0, j, neq))
        //       i -= 1

        auto adfs_count = maxw - minw; // number of adfs for one sadf t
        auto nadf = maxw; // data used is (maximum - first adf)
        auto neq_adf = maxw - order - 1; // number of equation for longest adf (maxw)
        // 1 sadf point requires at least GB (only matrix storage)
        auto xsize = (neq_adf * params) * 4; // 4 bytes float32
        auto sadft_GB = float(xsize * adfs_count / GIGABytes); // matrix storage for 1 sadf point
        auto nsadft = n - maxw; // number of sadf t's to calculate the entire SADF

        if (nsadft == 0) // 1 point SADF, needed by Metatrader 5
            nsadft = 1;

        auto batch_size = 1; // number of sadft points to calculate at once
        if (sadft_GB < gpumem_gb) { // each batch will have at least 1GB in OLS matrixes
            batch_size = int(gpumem_gb / sadft_GB);
            batch_size = (batch_size > nsadft) ? nsadft : batch_size; // in case bigger than sadf
        }

        auto nbatchs = nsadft / batch_size; // number of batchs to sent to GPU
        auto lst_batch_size = nsadft - nbatchs * batch_size; //last batch of adfs(integer%)

        if (verbose) {
            std::cout << "adfs_count " << adfs_count << std::endl;
            std::cout << "sadft_GB " << sadft_GB << std::endl;
            std::cout << "batch_size " << batch_size << std::endl;
            std::cout << "nsadft " << nsadft << std::endl;
            std::cout << "nbatchs " << nbatchs << std::endl;
            std::cout << "lst_batch_size " << lst_batch_size << std::endl;
        }

        // master X for a sadft (biggest adf OLS X matrix)
        auto Xm = th::zeros({ batch_size, neq_adf, params }, dtype_option.device(deviceCPU));
        // master Z for a sadft (biggest adf OLS independent term)
        auto zm = th::zeros({ batch_size, neq_adf }, dtype_option.device(deviceCPU));

        // pin memory to faster CPU to GPU  copy
        // trying to make GPU less idle so far not tested again
        //Xbt = Xbt.pin_memory();
        //zbt = zbt.pin_memory();
        //nobt = nobt.pin_memory();

        // CUDA (if existent) - same as above to batch operation
        auto Xcu = th::zeros({ batch_size * adfs_count, neq_adf, params }, dtype_option.device(deviceifGPU));
        auto zcu = th::zeros({ batch_size * adfs_count, neq_adf, 1 }, dtype_option.device(deviceifGPU));
        // result
        auto sadf = th::zeros({ nsadft }, dtype_option.device(deviceifGPU));
        auto adfmaxidx = th::zeros({ nsadft }, dtypeL_option.device(deviceifGPU));
        auto eye = th::eye(params, dtype_option.device(deviceifGPU));
        // statistical degrees of freedom for the t statistic test is allways the
        // number of equations - number of params (decrease as the length of data reduces)
        // the same based on the adf sequence inside a SADF(t) calculation
        //auto nobsc = (maxw - p - 1 - (p + 3) - th::arange(maxw - minw));
        auto dgfree = neq_adf - params - th::arange(maxw - minw);  //decreasing as we walk to minw
        auto dgfreecu = dgfree.repeat(batch_size).to(deviceifGPU);

        auto tline = 0; // start line for master main X OLS matrix/ z vector
        for (int i = 0; i < nbatchs; i++) {
            //# copy the batch_size first SADF(t) points ADF bigger matrices
            // for j in range(batch_size) :
            //        Xmb.select(0, j).copy_(X.narrow(0, t, nobsadf))
            //        zmb.select(0, j).copy_(z.narrow(0, t, nobsadf))
            //        t = t + 1
            for (int j = 0; j < batch_size; j++) { // assembly batch_size sadf'ts matrixes
                Xm.select(0, j).copy_(X.narrow(0, tline, neq_adf)); //  master X for this sadft - biggest adf OLS X matrix
                zm.select(0, j).copy_(z.narrow(0, tline, neq_adf)); // master Z for this sadft (biggest adf OLS independent term)
                tline++;
            }
            //# repeat those matrix for the number of ADF's of each SADF(t) point
            //Xcpu = Xmb.repeat([1, adfs_count, 1]).view(batch_size, adfs_count, nobsadf, p + 3)
            //zcpu = zmb.repeat([1, adfs_count]).view(batch_size, -1, nobsadf)
            auto Xcpu = Xm.repeat({ 1, adfs_count, 1 }).view({ batch_size, adfs_count, neq_adf, params });
            auto zcpu = zm.repeat({ 1, adfs_count }).view({ batch_size, -1, neq_adf });
            //for k in range(adfs_count) : # each is smaller than previous
            //    # Xbt[j, k, :k, : ] = 0
            //    # zbt[j, k, :k] = 0
            //    Xcpu.select(1, k).narrow(1, 0, k).fill_(0)
            //    zcpu.select(1, k).narrow(1, 0, k).fill_(0)
            for (int k = 0; k < adfs_count; k++) { //each is smaller than previous
                Xcpu.select(1, k).narrow(1, 0, k).fill_(0);
                zcpu.select(1, k).narrow(1, 0, k).fill_(0);
            }
            //Xcu.copy_(Xcpu.view(batch_size* adfs_count, nobsadf, (3 + p)))
            //zcu.copy_(zcpu.view(batch_size* adfs_count, nobsadf, 1))
            Xcu.copy_(Xcpu.view({ batch_size * adfs_count, neq_adf, params }), true);
            zcu.copy_(zcpu.view({ batch_size * adfs_count, neq_adf, 1 }), true);

            auto XcuT = Xcu.transpose(1, -1);
            auto L = Cholesky(XcuT.bmm(Xcu));
            auto xtz = XcuT.bmm(zcu);
            auto Bhat = th::cholesky_solve(xtz, L);
            auto Gi = th::cholesky_solve(eye, L); // (X ^ T.X) ^ -1
            auto er = zcu - Xcu.bmm(Bhat);
            Bhat = Bhat.squeeze();
            auto s2 = (er * er).sum(1).squeeze().div(dgfreecu);
            // tstats = Bhat[:, 2] / th.sqrt(s2 * Gi[:, 2, 2]) - for params = 3 // bellow generic
            auto adfstats = Bhat.select(-1, 2 - !drift).div(th::sqrt(Gi.select(-2, 2 - !drift).select(-1, 2 - !drift) * s2));
            //adfstats[th.isnan(adfstats)] = -3.4e+38
            adfstats.index_fill_(0, th::nonzero(th::isnan(adfstats)).view(-1), -3.4e+38); // in case colinearity causes singular matrices
            auto max = adfstats.view({ batch_size, adfs_count }).max(-1);

            sadf.narrow(0, tline - batch_size, batch_size).copy_(std::get<0>(max));
            adfmaxidx.narrow(0, tline - batch_size, batch_size).copy_(std::get<1>(max));
        }

        if (lst_batch_size > 0) {

            dgfreecu = dgfree.repeat(lst_batch_size).to(deviceifGPU);
            Xm = Xm.narrow(0, 0, lst_batch_size);
            zm = zm.narrow(0, 0, lst_batch_size);

            for (int j = 0; j < lst_batch_size; j++) { // assembly batch_size sadf'ts matrixes
                Xm.select(0, j).copy_(X.narrow(0, tline, neq_adf)); //  master X for this sadft - biggest adf OLS X matrix
                zm.select(0, j).copy_(z.narrow(0, tline, neq_adf)); // master Z for this sadft (biggest adf OLS independent term)
                tline++;
            }
            //# repeat those matrix for the number of ADF's of each SADF(t) point
            //Xcpu = Xmb.repeat([1, adfs_count, 1]).view(batch_size, adfs_count, nobsadf, p + 3)
            //zcpu = zmb.repeat([1, adfs_count]).view(batch_size, -1, nobsadf)
            auto Xcpu = Xm.repeat({ 1, adfs_count, 1 }).view({ lst_batch_size, adfs_count, neq_adf, params });
            auto zcpu = zm.repeat({ 1, adfs_count }).view({ lst_batch_size, -1, neq_adf });
            //for k in range(adfs_count) : # each is smaller than previous
            //    # Xbt[j, k, :k, : ] = 0
            //    # zbt[j, k, :k] = 0
            //    Xcpu.select(1, k).narrow(1, 0, k).fill_(0)
            //    zcpu.select(1, k).narrow(1, 0, k).fill_(0)
            for (int k = 0; k < adfs_count; k++) { //each is smaller than previous
                Xcpu.select(1, k).narrow(1, 0, k).fill_(0);
                zcpu.select(1, k).narrow(1, 0, k).fill_(0);
            }
            //Xcu.copy_(Xcpu.view(batch_size* adfs_count, nobsadf, (3 + p)))
            //zcu.copy_(zcpu.view(batch_size* adfs_count, nobsadf, 1))
            Xcu = Xcu.narrow(0, 0, lst_batch_size * adfs_count);
            zcu = zcu.narrow(0, 0, lst_batch_size * adfs_count);
            Xcu.copy_(Xcpu.view({ lst_batch_size * adfs_count, neq_adf, params }), true);
            zcu.copy_(zcpu.view({ lst_batch_size * adfs_count, neq_adf, 1 }), true);

            auto XcuT = Xcu.transpose(1, -1);
            auto L = Cholesky(XcuT.bmm(Xcu));
            auto xtz = XcuT.bmm(zcu);
            auto Bhat = th::cholesky_solve(xtz, L);
            auto Gi = th::cholesky_solve(eye, L); // (X ^ T.X) ^ -1
            auto er = zcu - Xcu.bmm(Bhat);
            Bhat = Bhat.squeeze();
            auto s2 = (er * er).sum(1).squeeze().div(dgfreecu);
            // tstats = Bhat[:, 2] / th.sqrt(s2 * Gi[:, 2, 2]) - for params = 3 // bellow generic
            auto adfstats = Bhat.select(-1, 2 - !drift).div(th::sqrt(Gi.select(-2, 2 - !drift).select(-1, 2 - !drift) * s2));
            //adfstats[th.isnan(adfstats)] = -3.4e+38
            adfstats.index_fill_(0, th::nonzero(th::isnan(adfstats)).view(-1), -3.4e+38); // in case colinearity causes singular matrices

            auto max = adfstats.view({ lst_batch_size, adfs_count }).max(-1);

            sadf.narrow(0, tline - lst_batch_size, lst_batch_size).copy_(std::get<0>(max));
            adfmaxidx.narrow(0, tline - lst_batch_size, lst_batch_size).copy_(std::get<1>(max));
        }

        sadf = sadf.to(deviceCPU);
        adfmaxidx = th::_cast_Float(adfmaxidx.to(deviceCPU));

        // adfs windows start bigger and get smaller to the end
        // adfmaxidx the closer to adfs_count smaller the window being used
        // so closer to the current point being calculated
        // so invert values meaning 0 is closer to the current point
        // and the bigger the value farther or bigger was the size of
        // ADF where the max value was found
        adfmaxidx = adfmaxidx.mul(-1).add(adfs_count);

        memcpy(outsadf, sadf.data_ptr<float>(), sizeof(float) * nsadft);
        memcpy(outadfmaxidx, adfmaxidx.data_ptr<float>(), sizeof(float) * nsadft);

        return nsadft;
    
#ifdef  DEBUG
        }
        catch (const std::exception& ex) {
            debugfile << "c++ exception: " << std::endl;
            debugfile << ex.what() << std::endl;
        }
        catch (...) {
            debugfile << "Weird no idea exception" << std::endl;
            //cmpbegin_time = -1;<
        }
#endif //  DEBUG

    // working perfectly - only one GPU so only one thread can access it a time
    // debugfile << "sadfd_mt5 unlocking thread" << std::endl;

        return -1;
}


int sadfd_mt5(double* signal, double* outsadf, double* lagout, int n, int maxw, int minw, int order, bool drift, double gpumem_gb, bool verbose) {
    float* fsignal = new float[n];
    float* foutsadf = new float[n - maxw];
    float* flagout = new float[n - maxw];

    float last_valid = 0;
    int count_evalues = 0; // count of invalid values filled
        
    try {
        // convert from double to float
        for (int i = 0; i < n; i++)
            fsignal[i] = (float)signal[i];

        auto ns = sadf(fsignal, foutsadf, flagout, n, maxw, minw, order, drift, (float)gpumem_gb, verbose);

        for (int i = 0; i < n - maxw; i++) {
            outsadf[i] = (double) foutsadf[i];
            lagout[i] = (double) flagout[i];
        }

        delete[] fsignal;
        delete[] foutsadf;
        delete[] flagout;
    }
    catch (const std::exception& ex) {
        debugfile << "c++ exception: " << std::endl;
        debugfile << ex.what() << std::endl;
    }
    catch (...) {
        debugfile << "Weird no idea exception" << std::endl;
    }

    return count_evalues;
}


